#!/usr/bin/env python3
"""Patch mGBA arm.c to add SP tracing"""

import sys
import os

arm_c_path = os.path.expanduser('~/mgba-instrumented/src/arm/arm.c')
backup_path = os.path.expanduser('~/mgba-instrumented/src/arm/arm.c.backup')

# Restore from backup first
with open(backup_path, 'r') as f:
    content = f.read()

# Find the position after the includes
insert_after = '#include <mgba/internal/arm/isa-thumb.h>'
patch_code = '''

// SP tracing for comparison with GBA emulator
static uint32_t g_min_sp = 0xFFFFFFFF;
static uint64_t g_instruction_count = 0;
static FILE* g_sp_log = NULL;

static void init_sp_logging(void) {
    if (!g_sp_log) {
        g_sp_log = fopen("/tmp/sp_trace_mgba.txt", "w");
        if (g_sp_log) {
            fprintf(g_sp_log, "# instruction_count,PC,SP,mode,min_sp\\n");
        }
    }
}

static void log_sp(struct ARMCore* cpu, const char* mode) {
    init_sp_logging();
    g_instruction_count++;
    
    uint32_t sp = cpu->gprs[13];
    if (sp < g_min_sp && sp >= 0x03000000 && sp < 0x03008000) {
        g_min_sp = sp;
    }
    
    if (g_sp_log && (g_instruction_count % 10000 == 0)) {
        fprintf(g_sp_log, "%llu,0x%08X,0x%08X,%s,0x%08X\\n", 
                (unsigned long long)g_instruction_count, 
                cpu->gprs[15], sp, mode, g_min_sp);
        fflush(g_sp_log);
    }
}
'''

# Insert the patch after includes
new_content = content.replace(insert_after, insert_after + patch_code)

# Also add log_sp calls in ARMRunLoop
# Find ThumbStep and ARMStep calls and add logging before them
new_content = new_content.replace(
    'ThumbStep(cpu);',
    'log_sp(cpu, "THUMB"); ThumbStep(cpu);'
)
new_content = new_content.replace(
    'ARMStep(cpu);',
    'log_sp(cpu, "ARM"); ARMStep(cpu);'
)

# Write back
with open(arm_c_path, 'w') as f:
    f.write(new_content)

print(f"Patch applied to {arm_c_path}")
print("SP tracing will be written to /tmp/sp_trace_mgba.txt")
