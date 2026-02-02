# Next Context Prompt for REAPER Development

## Task Completed
✅ README.md has been successfully rewritten to reflect REAPER's identity as a standalone game project

## Current State
- README.md now clearly describes REAPER as a new game built on PsyDoom engine
- Documentation files initialized: PROGRESS-LOG.MD, AGENT-REFERENCE.MD, TODO.TXT
- Project branding updated in main documentation

## Suggested Next Steps

### High Priority
1. **Update CMakeLists.txt**: Change project name from "PsyDoom" to "REAPER"
2. **Update source code headers**: Replace PsyDoom references in code files with REAPER branding
3. **Launcher branding**: Update any launcher UI text/titles to reference REAPER

### Medium Priority  
4. **Asset pipeline documentation**: Document the process for creating and importing custom assets
5. **Engine customization plan**: Identify which PsyDoom features to keep/modify/remove for REAPER
6. **First custom asset**: Create a simple test asset (texture or sprite) to validate the pipeline

### Context for Next Agent
- This is a **fork** of PsyDoom, not a mod - we're making a completely new game
- Maintain C-FORM coding standards (see .github/instructions/cform.instructions.md)
- Always update TODO.TXT when tasks are completed
- Log detailed progress in PROGRESS-LOG.MD
- Update AGENT-REFERENCE.MD with important discoveries or decisions

### Files to Check
- `CMakeLists.txt` - Still references "PsyDoom" as project name
- `game/` directory - Contains source files that may need header updates
- `docs/` directory - All documentation files for tracking progress

Remember: REAPER aims for 100% custom assets while maintaining the PSX aesthetic!
