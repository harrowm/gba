# Large Trace File Storage

## The 149MB trace file is NOT in git

The trace file `traces/mgba_sonic_10M_instructions.log.gz` (149 MB) is stored locally only and excluded from git via `.gitignore`.

## Why Not in Git?

- File is 149 MB (too large for efficient git storage)
- Git is designed for source code, not large binary/compressed files
- Would slow down clone/pull operations significantly

## How to Share the Trace

If you need to share the trace file with others, use one of these methods:

### Option 1: Cloud Storage (Recommended)
Upload to:
- Google Drive
- Dropbox
- OneDrive
- AWS S3
- GitHub Releases (as an asset, not in repo)

### Option 2: Git LFS (Large File Storage)
If you really need it in git:

```bash
# Install git-lfs
brew install git-lfs  # macOS
# or: apt-get install git-lfs  # Linux

# Initialize git-lfs
git lfs install

# Track the trace file type
git lfs track "traces/*.log.gz"

# Add and commit
git add .gitattributes traces/mgba_sonic_10M_instructions.log.gz
git commit -m "Add trace file via Git LFS"
git push
```

### Option 3: Regenerate When Needed
The trace can be regenerated using:
```bash
# Follow instructions in docs/mgba_trace_collection.md
# Takes ~2-3 hours to collect 10M instructions
```

## Current Setup

- ✅ Trace file stored locally: `traces/mgba_sonic_10M_instructions.log.gz`
- ✅ Documentation committed: `traces/README.md`
- ✅ Analysis committed: `docs/SONIC_BOOT_COMPLETE_ANALYSIS.md`
- ✅ `.gitignore` configured to exclude trace files
- ✅ Git repository is clean and ready to push

## Recommendation

**Keep the trace file local**. The documentation and analysis are in git, which is what matters for understanding the boot sequence. The trace file can be regenerated if needed.
