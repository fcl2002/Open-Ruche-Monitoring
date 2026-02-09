# Open Ruche - Monitoring d'une ruche d'abeille 🐝

> **About this document:**  
> This file describes the **development rules and standards** for the Open Ruche project. It serves as a **development manual for collaborative teamwork**, ensuring consistency in code contributions, version control practices, and project organization.

---

## Project Structure

The project is divided into two main modules:

### Hardware (`/hardware`)

Contains all files related to physical system development:

```
hardware/
├── schematics/          # Electrical schematics (KiCad, Eagle, etc)
├── pcb/                 # PCB layout and Gerber files
├── bom/                 # Bill of Materials (component list)
├── cad/                 # 3D models for enclosures and structures
└── datasheets/          # Component datasheets
```

### Software (`/software`)

Contains all source code, divided by platform and functionality:

```
software/
├── arduino-ia/          # Arduino code with embedded AI
│   ├── src/            # Main source code
│   ├── lib/            # Specific libraries
│   ├── models/         # Converted AI models (TensorFlow Lite)
│   └── platformio.ini  # PlatformIO configuration
│
├── xiao-ia/            # XIAO code with embedded AI
│   ├── src/            # Main source code
│   ├── lib/            # Specific libraries
│   ├── models/         # Converted AI models
│   └── platformio.ini  # PlatformIO configuration
│
├── lora-communication/ # LoRaWAN communication module
│   ├── src/            # LoRa drivers and protocols
│   ├── payloads/       # Payload structures
│   └── tests/          # Range and connectivity tests
│
├── sensors/            # Sensor drivers and libraries
│   ├── temperature/    # Temperature sensor
│   ├── humidity/       # Humidity sensor
│   ├── weight/         # Weight sensor (HX711)
│   ├── sound/          # Microphone and audio analysis
│   └── vision/         # Camera and image processing
│
├── interfaces/         # Display interfaces (affichage)
│   ├── display-local/  # Local LCD/OLED display
│   ├── web-dashboard/  # Web dashboard
│   └── mobile-app/     # Mobile app (optional)
│
└── cloud/              # Cloud platform integration
    ├── datacake/       # Datacake configuration
    ├── beep/           # BEEP Monitor API
    └── ttn/            # The Things Network
```

### Documentation and Others

```
docs/                   # Technical documentation
├── architecture.md     # System architecture
├── specifications.md   # Technical specifications
├── user-guide.md       # User manual
└── tests-report.md     # Test reports

ai-training/            # AI training scripts (outside embedded)
├── vision/             # Computer vision training
│   ├── dataset/       # Image dataset
│   ├── training/      # Training scripts
│   └── models/        # Trained models
│
└── audio/              # Audio analysis training
    ├── dataset/       # Audio dataset
    ├── training/      # Training scripts
    └── models/        # Trained models

assets/                 # Project resources
├── images/            # Images and photos
├── diagrams/          # Diagrams and schemes
└── videos/            # Demo videos
```

---

## Commit and Branch Standards

### Commit Convention

We use the **Conventional Commits** standard to maintain a clean and semantic history.

#### Format:

```
<type>(<scope>): <short description>

```

#### Commit Types:

- **feat**: New feature
- **fix**: Bug fix
- **hw**: Hardware changes (schematic, PCB, CAD)
- **ai**: Machine learning/AI related work
- **docs**: Documentation changes
- **style**: Code formatting (no logic change)
- **refactor**: Code refactoring
- **test**: Adding or modifying tests
- **chore**: Maintenance tasks (build, CI/CD, etc)
- **perf**: Performance improvements

#### Suggested Scopes:

- `arduino-ai` - Arduino code
- `xiao-ai` - XIAO code
- `lora` - LoRaWAN communication
- `sensors` - Sensors (weight, temperature, etc)
- `display` - Local interface
- `dashboard` - Web interface
- `schematic` - Electrical schematic
- `pcb` - PCB layout
- `vision` - Computer vision AI
- `audio` - Audio analysis AI
- `cloud` - Cloud integration

#### Commit Examples:

```bash
# New feature
git commit -m "feat(sensors): add HX711 weight sensor driver"

# Bug fix
git commit -m "fix(lora): fix OTAA connection timeout"

# Hardware
git commit -m "hw(pcb): update layout rev 1.2 with new connector"

# AI
git commit -m "ai(vision): implement Asian hornet detection with TensorFlow Lite"

# Documentation
git commit -m "docs(architecture): add system block diagram"

# Commit with descriptive body
git commit -m "feat(arduino-ia): add local AI inference

- Implement image preprocessing pipeline
- Integrate converted TensorFlow Lite model
- Add circular buffer for memory optimization

Closes #23"
```

### Branch Strategy

We use a simplified **Git Flow**:

#### Main Branches:

- **`main`**: Production code, always stable
  - Protected: requires approved Pull Request
  - Only releases and hotfixes are merged here
- **`develop`**: Development branch, continuous integration
  - Base for all features
  - Must always be functional

#### Working Branches:

##### Feature Branches

For new features:

```bash
git checkout develop
git checkout -b feature/<feature-name>

# Examples:
git checkout -b feature/temperature-sensor
git checkout -b feature/web-dashboard
git checkout -b feature/hornet-detection-model
```

##### Hardware Branches

For hardware work:

```bash
git checkout develop
git checkout -b hw/<description>

# Examples:
git checkout -b hw/schematic-rev2
git checkout -b hw/pcb-layout-solar
git checkout -b hw/case-3d-design
```

##### Bugfix Branches

For bug fixes:

```bash
git checkout develop
git checkout -b fix/<bug-description>

# Examples:
git checkout -b fix/lora-timeout
git checkout -b fix/sensor-incorrect-reading
```

##### Hotfix Branches

For urgent production fixes:

```bash
git checkout main
git checkout -b hotfix/<version>

# Example:
git checkout -b hotfix/v1.0.1
```

### Merge Workflow

#### 1. Working on a Feature:

```bash
# 1. Create branch from develop
git checkout develop
git pull origin develop
git checkout -b feature/my-feature

# 2. Make commits following the convention
git add .
git commit -m "feat(scope): change description"

# 3. Keep branch updated
git checkout develop
git pull origin develop
git checkout feature/my-feature
git merge develop  # or: git rebase develop

# 4. Push the branch
git push origin feature/my-feature
```

#### 2. Creating a Pull Request:

- Create PR from `feature/my-feature` → `develop`
- PR title: Similar to main commit
- Description: Explain what was done and why
- Add team reviewers
- Wait for approval and CI to pass

#### 3. Merge to Main (Release):

```bash
# When develop is ready for release:
git checkout main
git merge develop --no-ff -m "release: v1.0.0"
git tag -a v1.0.0 -m "Release version 1.0.0"
git push origin main --tags
```

#### 4. Hotfix:

```bash
# 1. Create hotfix from main
git checkout main
git checkout -b hotfix/v1.0.1

# 2. Make the fix
git commit -m "fix(lora): fix critical reconnection failure"

# 3. Merge to both main AND develop
git checkout main
git merge hotfix/v1.0.1 --no-ff
git tag -a v1.0.1 -m "Hotfix version 1.0.1"

git checkout develop
git merge hotfix/v1.0.1 --no-ff

git push origin main develop --tags
git branch -d hotfix/v1.0.1
```

### Important Rules:

✅ **Never** commit directly to `main` or `develop`  
✅ **Always** create a branch to work on  
✅ **Always** pull from `develop` before creating a new branch  
✅ **Always** test before pushing  
✅ **Always** keep commits small and focused  
✅ **Always** write descriptive messages

❌ **Never** push credentials or secrets  
❌ **Never** commit large binary files without .gitignore  
❌ **Never** merge without approval on protected branches
