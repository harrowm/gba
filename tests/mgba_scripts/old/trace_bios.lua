-- mGBA CPU State Logger
-- Logs CPU state at the end of each frame for comparison

-- Configuration
local MAX_FRAMES = 10
local LOG_FILE = "/tmp/mgba_bios_trace.log"

-- Key memory addresses to monitor
local WATCH_ADDRESSES = {
    0x03000000, -- WRAM start
    0x03007FFC, -- WRAM end
    0x04000000, -- DISPCNT
    0x04000004, -- DISPSTAT
    0x04000006, -- VCOUNT
    0x04000008, -- BG0CNT
    0x04000200, -- IE (Interrupt Enable)
    0x04000202, -- IF (Interrupt Flag)
    0x04000204, -- WAITCNT
    0x04000300, -- POSTFLG (BIOS POST flag)
}

-- Initialize counters
local frame_count = 0
local log_file = io.open(LOG_FILE, "w")

-- Helper function to format hex values
local function hex32(val)
    return string.format("0x%08X", val)
end

local function hex16(val)
    return string.format("0x%04X", val)
end

local function hex8(val)
    return string.format("0x%02X", val)
end

-- Function to read memory safely
local function safe_read(address, size)
    local success, value = pcall(function() return emu:read32(address) end)
    if success then
        return value
    else
        return 0
    end
end

-- Function to log CPU state
local function log_cpu_state()
    -- Get CPU registers
    local r = {}
    for i = 0, 15 do
        r[i] = emu.readRegister("r" .. i)
    end
    
    -- Get CPSR (Current Program Status Register)
    local cpsr = emu.readRegister("cpsr")
    
    -- Get SPSR (Saved Program Status Register) - might not be valid in all modes
    local spsr = emu.readRegister("spsr")
    
    -- Log instruction count and PC
    log_file:write(string.format("=== Instruction %d ===\n", instruction_count))
    log_file:write(string.format("PC: %s\n", hex32(r[15])))
    
    -- Log general purpose registers
    log_file:write("Registers:\n")
    for i = 0, 12 do
        log_file:write(string.format("  R%d: %s", i, hex32(r[i])))
        if i % 4 == 3 then
            log_file:write("\n")
        else
            log_file:write("  ")
        end
    end
    if r[12] ~= 0 or r[11] ~= 0 or r[10] ~= 0 then
        if r[12] == 0 and r[11] == 0 then
            log_file:write("\n")
        end
    else
        log_file:write("\n")
    end
    
    -- Log special registers
    log_file:write(string.format("  SP: %s  LR: %s  PC: %s\n", 
        hex32(r[13]), hex32(r[14]), hex32(r[15])))
    
    -- Log CPSR
    log_file:write(string.format("CPSR: %s (N=%d Z=%d C=%d V=%d I=%d F=%d T=%d Mode=0x%X)\n",
        hex32(cpsr),
        bit32.extract(cpsr, 31, 1), -- N
        bit32.extract(cpsr, 30, 1), -- Z  
        bit32.extract(cpsr, 29, 1), -- C
        bit32.extract(cpsr, 28, 1), -- V
        bit32.extract(cpsr, 7, 1),  -- I
        bit32.extract(cpsr, 6, 1),  -- F
        bit32.extract(cpsr, 5, 1),  -- T
        bit32.extract(cpsr, 0, 5)   -- Mode
    ))
    
    -- Log SPSR if different from CPSR
    if spsr ~= cpsr then
        log_file:write(string.format("SPSR: %s\n", hex32(spsr)))
    end
    
    -- Log watched memory addresses
    log_file:write("Memory:\n")
    for _, addr in ipairs(WATCH_ADDRESSES) do
        local value = safe_read(addr, 4)
        log_file:write(string.format("  %s: %s", hex32(addr), hex32(value)))
        if addr % 0x10 == 0xC then
            log_file:write("\n")
        else
            log_file:write("  ")
        end
    end
    log_file:write("\n\n")
    log_file:flush()
end

-- Main execution hook
local function step_instruction()
    if instruction_count >= MAX_INSTRUCTIONS then
        emu.pause()
        log_file:write("Completed " .. MAX_INSTRUCTIONS .. " instructions.\n")
        log_file:close()
        print("BIOS tracing completed! Check " .. LOG_FILE)
        return
    end
    
    -- Step one instruction
    emu.step()
    instruction_count = instruction_count + 1
    
    -- Log the state after this instruction
    log_cpu_state()
    
    -- Continue execution
    emu.frameAdvance()
end

-- Initialize and start
print("Starting BIOS instruction trace...")
print("Tracing first " .. MAX_INSTRUCTIONS .. " instructions")
print("Output will be saved to " .. LOG_FILE)

-- Clear any existing log
if log_file then
    log_file:write("mGBA BIOS Instruction Trace Log\n")
    log_file:write("==============================\n\n")
    log_file:flush()
else
    error("Failed to create log file: " .. LOG_FILE)
end

-- Set up callbacks using mGBA's actual API
callbacks:add("frame", step_instruction)

-- Start execution
console:log("Script loaded! Execution will begin automatically.")