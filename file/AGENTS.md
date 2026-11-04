# AGENTS.md --- KDE Connect Enhanced

This file defines mandatory working instructions for AI coding agents
operating in this repository.

## 1. Mission

Enhance the existing KDE Connect desktop application with a more modern,
polished, accessible, and intuitive user experience.

This is an **existing-codebase enhancement**, not a rewrite.

The primary product requirements are defined in [`PRD.md`](./PRD.md).
Read that document before planning or implementing work.

## 2. Mandatory First Steps

Before editing any source file:

1.  Read `PRD.md`, repository `README` files, contribution guidelines,
    and relevant developer documentation.
2.  Inspect the current Git status. Do not overwrite or discard the
    user's uncommitted changes.
3.  Identify the current branch, remotes, and repository revision.
4.  Inspect the project structure and determine the actual languages, UI
    framework, Qt/KDE Frameworks versions, build system, application
    entry points, plugin system, D-Bus integration, and test setup.
5.  Read the relevant source files before proposing modifications.
6.  Build the unmodified project using its documented process, if the
    environment has the required dependencies.
7.  Run relevant existing tests and record the baseline results.

**Initial audit mode:** If no implementation task has been explicitly
approved, do not modify source files. Produce an audit report and a
proposed implementation plan first.

Never assume a framework, directory structure, command, or dependency
without verifying it in the repository.

## 3. Non-Negotiable Project Rules

-   Preserve the existing KDE Connect architecture wherever practical.
-   Preserve the KDE Connect communication protocol.
-   Preserve compatibility with the official KDE Connect Android
    application.
-   Preserve existing device pairing, authentication, and trust
    behavior.
-   Preserve existing plugins and user configuration.
-   Reuse existing services, models, events, signals, and plugin APIs.
-   Do not introduce a second source of truth for device or connection
    state.
-   Do not replace the backend or rewrite the application in an
    unrelated framework.
-   Do not remove existing functionality to simplify implementation.
-   Do not use mock data in production features.
-   Do not claim that an unsupported feature is available.
-   Do not introduce unnecessary dependencies or background services.
-   Do not make broad unrelated refactors while implementing a focused
    feature.
-   Preserve copyright notices, attribution, and applicable license
    requirements.

If a requested change conflicts with the current architecture, explain
the conflict and recommend the least disruptive solution before
proceeding.

## 4. Source Code and Git Safety

Before making changes:

-   Check `git status`.
-   Identify and preserve user changes.
-   Do not run destructive Git commands such as `reset --hard`,
    `clean -fd`, or forced checkout unless the user explicitly
    authorizes the exact operation.
-   Do not overwrite user files or local configuration.
-   Do not modify the system-installed KDE Connect during development.
-   Prefer a dedicated feature branch or Git worktree.
-   Keep commits small and focused.
-   Never commit credentials, tokens, private keys, personal data, or
    local machine secrets.
-   Do not rewrite Git history or force-push without explicit
    authorization.

Suggested branches, adapted to the repository's workflow:

``` text
main
development
feature/dashboard-redesign
feature/device-management
feature/file-transfer
feature/plugin-manager
feature/settings
hotfix/*
```

## 5. Architecture and Implementation

### 5.1 Inspect before extending

Determine how the current project implements:

-   Application startup and lifecycle.
-   Main window and navigation.
-   Device discovery and device models.
-   Pairing and connection state.
-   D-Bus interfaces and service communication.
-   Plugin discovery, configuration, and lifecycle.
-   File transfer.
-   Notifications.
-   Clipboard synchronization.
-   Media control and remote input.
-   Settings and persistence.
-   Translations and accessibility.
-   Packaging and installation.

Use the project's established patterns and APIs.

### 5.2 UI changes

The desired visual direction is:

**Modern Linux desktop + premium minimalism + subtle futuristic
details.**

Priorities:

-   Clear information hierarchy.
-   Consistent spacing and alignment.
-   Readable typography.
-   Restrained accent colors.
-   Consistent iconography.
-   Polished light and dark themes where supported.
-   Subtle, efficient animations.
-   Clear loading, empty, success, unavailable, and error states.
-   Keyboard navigation and visible focus.
-   Respect for KDE and host desktop conventions.

Do not make the application look like an unrelated web dashboard.
Preserve a coherent native desktop experience.

### 5.3 State and data

-   All device information must come from real KDE Connect services or
    models.
-   Use existing event-driven updates where possible.
-   Avoid unnecessary polling.
-   Handle missing, stale, or unavailable device data explicitly.
-   Do not invent battery, connection, transfer, or activity values.
-   Keep UI state synchronized with the authoritative application state.

## 6. Feature Priorities

Follow the priority order in `PRD.md`.

### P0 --- Initial release foundation

1.  Repository audit and baseline build.
2.  Design system and UI foundation.
3.  Dashboard / overview.
4.  Device management.
5.  File-transfer experience.
6.  Plugin manager improvements.
7.  Regression testing.

### P1 --- Subsequent improvements

1.  Notification experience.
2.  Clipboard experience.
3.  Media and remote-control presentation.
4.  Settings organization.
5.  Accessibility and visual polish.

### P2 --- Separately evaluated enhancements

1.  System monitoring.
2.  Advanced command features.

Do not start P1 or P2 work while P0 foundations are unstable unless the
user explicitly changes priorities.

## 7. Security and Privacy

Security is mandatory.

-   Do not bypass device pairing or authentication.
-   Do not weaken trusted-device verification.
-   Do not expose private services or D-Bus interfaces to untrusted
    networks.
-   Validate all external input.
-   Avoid logging notification contents, clipboard contents,
    credentials, or private file data.
-   Do not persist private content unless explicitly required and
    appropriately protected.
-   Respect existing permission and consent mechanisms.
-   Do not run arbitrary commands received from a remote device.
-   Do not run commands as root by default.
-   Do not add unrestricted remote shell access.

Any future command-execution capability must be separately designed and
reviewed. It must use explicit opt-in, authenticated devices, a
restricted allowlist, suitable confirmation for sensitive actions, and
revocation controls.

## 8. Performance and Reliability

-   Keep the UI thread responsive.
-   Avoid blocking operations in UI code.
-   Avoid unnecessary polling and repeated service queries.
-   Avoid excessive animations and expensive rendering.
-   Handle network loss and device reconnection safely.
-   Handle unavailable plugins and missing device capabilities
    gracefully.
-   Avoid unnecessary memory retention.
-   Measure performance before and after significant changes.

The application must remain usable on modest Linux desktop hardware.

## 9. Required Development Workflow

For each feature, follow these steps in order.

### Step A --- Understand

Read the relevant PRD section and inspect the current implementation.

### Step B --- Plan

Before editing, report:

-   Feature objective.
-   Current behavior.
-   Proposed behavior.
-   Relevant source files and components.
-   Existing APIs or services to reuse.
-   Dependencies, if any.
-   Risks and compatibility implications.
-   Test plan.
-   Acceptance criteria.

### Step C --- Implement

-   Make the smallest coherent change.
-   Follow repository coding conventions.
-   Reuse existing components and APIs.
-   Keep presentation changes separate from protocol changes.
-   Add or update tests.
-   Update translations and documentation where appropriate.

### Step D --- Verify

-   Run formatting or lint checks if configured.
-   Build the relevant targets.
-   Run relevant unit and integration tests.
-   Run manual checks where automation is insufficient.
-   Test related existing functionality for regressions.
-   Inspect the final Git diff.

### Step E --- Report

Use the reporting format in Section 13.

Do not claim success when a command, build, or test has not actually
completed successfully.

## 10. Testing Requirements

Test applicable scenarios, including:

-   Application startup and shutdown.
-   Device discovery.
-   Pairing approval and rejection.
-   Unpairing.
-   Device disconnection and reconnection.
-   Multiple devices.
-   Missing battery information.
-   File-transfer success and failure.
-   Large and interrupted transfers.
-   Plugin enablement and configuration.
-   Notification synchronization.
-   Clipboard synchronization.
-   Media controls.
-   Theme switching.
-   Keyboard navigation.
-   Empty states.
-   Permission and unavailable states.

Do not replace existing tests with weaker tests just to make the build
pass.

When a test cannot be run, state why and identify the remaining
verification needed.

## 11. Build and Installation Safety

-   Follow the build instructions in the repository.
-   Use an isolated build directory.
-   Keep build output separate from source files.
-   Prefer a local development installation or other isolated test
    environment.
-   Do not overwrite the user's system package.
-   Do not remove or replace the installed KDE Connect service.
-   Do not use `sudo` unless necessary and explicitly explained.
-   Before any system-level installation or removal, describe the exact
    effects and request authorization.
-   Provide rollback instructions for installation changes.

Only prepare distribution formats supported by the project's actual
dependency and packaging requirements.

## 12. Documentation Requirements

Keep relevant documentation updated.

Expected project documents:

-   `PRD.md` --- product requirements.
-   `AGENTS.md` --- coding-agent instructions.
-   `ARCHITECTURE.md` --- verified architecture and data flow.
-   `DESIGN_SYSTEM.md` --- design tokens and visual rules.
-   `UI_UX_GUIDELINES.md` --- interaction and accessibility guidance.
-   `ROADMAP.md` --- development phases.
-   `TESTING.md` --- build and test instructions.
-   `CHANGELOG.md` --- notable changes.
-   `UPSTREAM_SYNC.md` --- upstream tracking and synchronization notes.

Do not invent architecture documentation. Record only verified facts and
clearly label unresolved questions.

## 13. Required Progress Report

After every implementation task, provide:

``` text
## Implementation Report

### Objective
What was requested?

### Changes
What was implemented?

### Files Modified
List files and explain why they changed.

### Technical Decisions
Explain important design and architecture decisions.

### Build
Command executed and actual result.

### Tests
Tests executed and actual result.

### Compatibility
Impact on KDE Connect core, plugins, protocol, and Android compatibility.

### Known Issues
Remaining bugs, limitations, or unverified behavior.

### Git Status
Branch and summary of the final diff.

### Next Recommended Task
One clearly justified next step.
```

Distinguish clearly between:

-   Implemented.
-   Built.
-   Tested.
-   Manually verified.
-   Not yet verified.

These are different completion states.

## 14. Definition of Done

A change is complete only when:

-   It satisfies the relevant PRD requirements.
-   It follows the existing architecture.
-   It uses real application state.
-   It preserves related existing functionality.
-   Relevant tests are added or updated.
-   The project builds successfully, where the environment permits.
-   Relevant tests pass, or unrun tests are explicitly disclosed.
-   Security and privacy implications are reviewed.
-   Documentation is updated.
-   The final diff contains no unrelated changes.
-   The implementation report is provided.

## 15. First Task for a New Agent

When first entering this repository:

1.  Read `PRD.md` and this `AGENTS.md`.
2.  Inspect repository status and structure.
3.  Read project build and contribution documentation.
4.  Identify the actual architecture, UI framework, and dependencies.
5.  Build the unmodified source if possible.
6.  Run existing tests if possible.
7.  Produce a source-code audit and implementation proposal.
8.  Recommend the smallest safe first UI improvement.

**Do not edit source code during this first audit. Wait for the user to
approve the implementation plan.**

------------------------------------------------------------------------

## Final Rule

**Improve the existing KDE Connect application. Do not replace it.
Preserve what works, verify every change, and prioritize a safe,
maintainable, compatible result over visual novelty.**
