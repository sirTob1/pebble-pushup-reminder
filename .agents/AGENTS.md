# Local Agent Rules

All local rules for this repository have been migrated to the Global Customizations Root (`C:\Users\twigb\.gemini\config\AGENTS.md`) to apply universally across all repositories. 
If new project-specific rules are needed in the future, they can be added here.

- **Pebble Versioning Constraint:** Pebble apps only support `<major>.<minor>` version numbers (e.g., `2.2`). Never use a third patch number (e.g., `2.2.1`) for the app's version, `CHANGELOG.md` releases, or the `versionLabel` in `package.json`.
