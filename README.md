# 👩‍💻 Team Contribution Breakdown — AdvancedEMS

This document outlines how the AdvancedEMS codebase is divided among team members for individual contribution tracking, code review, and academic assessment.

---

## 🗂 Module Overview

```
AdvancedEMS
├── Module 1 — Employee Directory         → Talha Sajjad
├── Module 2 — Biometric & Attendance     → Habiba Farid
├── Module 3 — Payroll & Reports          → Aina Bakhtawer
└── Module 4 — Role-Based Security        → Syeda Kashaf Fatima
```

---

---

## 👤 Module 1 — Employee Directory
**Contributor: Talha Sajjad**

### Responsibility
Design and implement the core employee data model, the persistence layer (file-based database), and all UI interactions for managing the employee directory including search, filter, sort, add, edit, and delete.

### Files Owned

| File | Role |
|------|------|
| `lib/Employee.h` | Employee class declaration — all fields, getters/setters, attendance counters, net pay interface |
| `lib/Employee.cpp` | Employee constructor, `calculateNetPay()`, `generateSlipPayload()` implementations |
| `lib/Database.h` | Database class declaration — `FilterCriteria` / `DepartmentStats` structs, all CRUD and analytics method signatures |
| `lib/Database.cpp` | Full database implementation — file I/O, `insertRecord`, `removeRecord`, `modifyRecord`, `lookupRecord`, all three `searchDatabase` overloads, five sort methods, payroll analytics |
| `gui/Dashboard.cpp` *(Employees page section)* | `buildEmployeesPage()`, `populateGrid()`, `reloadViewGrid()`, `computeMetrics()`, `applyFiltersAndRefresh()`, `executionAddWorkflow()`, `executionUpdateWorkflow()`, `executionDeleteWorkflow()`, `executionLiveSearch()`, `executionAdvancedFilter()`, `executionSortChanged()`, `executionExportWorkflow()`, `onPrintEmployeeList()` |

### Key OOP Concepts Applied
- **Encapsulation**: All Employee fields are `private`; data is accessed exclusively through typed getters/setters.
- **Static Polymorphism (Overloading)**: `Database::searchDatabase()` is overloaded with three distinct signatures (text query, minimum salary, full `FilterCriteria`).
- **Abstraction**: `FilterCriteria` and `DepartmentStats` structs cleanly separate query parameters from result data.

### Core Features Delivered
- Complete employee profile (ID, name, department, designation, CNIC, gender, email, phone, join date, status)
- Live real-time search as the user types
- Advanced multi-field filter dialog (salary range, department, designation, status, gender, attendance %)
- Five sort modes: alphabetical, salary rank, attendance %, department, ID
- CSV export of the current filtered view
- Direct print of the employee table via Qt PrintSupport
- Metric cards: total staff, total payroll, average attendance, active count

---

---

## 👤 Module 2 — Biometric & Attendance
**Contributor: Habiba Farid**

### Responsibility
Design and implement all biometric dialogs — fingerprint (hardware SDK stub) and Face ID (live webcam, Qt Multimedia) — covering both enrollment during onboarding and verification for attendance marking.

### Files Owned

| File | Role |
|------|------|
| `gui/FingerprintDialog.h` | Dialog class declaration — `Mode` enum (Enroll / Verify), public API (`wasVerified`, `markType`, `enrolledTemplate`), UI element declarations |
| `gui/FingerprintDialog.cpp` | Full fingerprint dialog implementation — UI build, animated scan progress, SDK stub methods (`simulateEnroll`, `simulateScan`), mark-type combo (Present / Absent / On Leave) |
| `gui/FaceIdDialog.h` | Face ID dialog + `SpinRing` overlay widget declarations — camera pipeline, `QFutureWatcher` for async matching, face detection rect |
| `gui/FaceIdDialog.cpp` | Full Face ID implementation — `SpinRing` animation, Qt Multimedia camera capture, Haar-like face detection, RGB histogram chi-square matching, enrollment flow, verification flow |

### Key OOP Concepts Applied
- **Encapsulation**: Both dialogs expose a clean, minimal public API; all hardware and camera logic is hidden as private members.
- **Abstraction**: The SDK integration is isolated inside two stub methods (`simulateEnroll`, `simulateScan`) — replacing them with real SDK calls requires no changes elsewhere.

### Core Features Delivered
- `FingerprintDialog` in **Enroll** mode: captures and returns a biometric template string for storage
- `FingerprintDialog` in **Verify** mode: validates finger scan, returns verification result and mark type
- `FaceIdDialog` in **Enroll** mode: opens live webcam, detects face, captures Base64-encoded JPEG template
- `FaceIdDialog` in **Verify** mode: compares live frame against stored template using chi-square distance (configurable `MATCH_THRESHOLD`)
- `SpinRing` custom widget: animated 360° ring overlay that turns green on match and red on failure
- Attendance counters (present / absent / leave) updated in real time upon successful verification

---

---

## 👤 Module 3 — Payroll & Reports
**Contributor: Aina Bakhtawer**

### Responsibility
Design and implement the payroll computation model (including manager bonuses), pay slip generation, all four report types, and print/PDF export functionality.

### Files Owned

| File | Role |
|------|------|
| `lib/Manager.h` | `ExecutiveManager` class — inherits `Employee`, adds `managementBonus` and `directReports`; overrides `calculateNetPay()` and `generateSlipPayload()` |
| `lib/Employee.cpp` *(payroll methods)* | `calculateNetPay()` — Basic Salary + Allowance − Deductions; `generateSlipPayload()` — formatted pay slip text block |
| `gui/Dashboard.cpp` *(Reports page section)* | `buildReportsPage()`, `buildFullReport()`, `buildDeptReport()`, `buildAttendanceReport()`, `buildPayrollReport()`, `onGenerateFullReport/Dept/Attendance/Payroll()`, `onExportReport()`, `onPrintReport()`, `onExportReportPDF()`, `printTextContent()`, `printTableWidget()`, `exportContentToPDF()`, `reloadDeptStats()`, `executionPaySlipWorkflow()` |

### Key OOP Concepts Applied
- **Inheritance**: `ExecutiveManager` extends `Employee` transparently — all Employee APIs work on managers without change.
- **Polymorphism (Runtime)**: `calculateNetPay()` and `generateSlipPayload()` are `virtual` in `Employee` and overridden in `ExecutiveManager` to include the management bonus.

### Core Features Delivered
- Net pay formula: `basicSalary + allowance − deductions` (base); `+ managementBonus` for Executive Managers
- Pay slip generator: formatted plain-text block with employee details, salary breakdown, and net pay
- **Full Summary Report**: organisation-wide headcount, payroll, attendance overview
- **Department Report**: per-department table — head count, total payroll, average salary, average attendance
- **Attendance Report**: ranked list of all employees by attendance percentage
- **Payroll Report**: ranked list by net pay with salary breakdown
- Export report to plain `.txt` file
- Export report to PDF (via Qt PrintSupport `QPrinter`)
- Direct print of report text and department stats table

---

---

## 👤 Module 4 — Role-Based Security
**Contributor: Syeda Kashaf Fatima**

### Responsibility
Design and implement the user authentication system, role hierarchy, session management, and the login UI — including enforcing access restrictions throughout the dashboard based on the logged-in role.

### Files Owned

| File | Role |
|------|------|
| `lib/User.h` | Abstract `User` base class + `AdminUser`, `HRUser`, `ViewerUser` concrete classes; `getSystemAccessRights()` pure virtual method |
| `lib/UserDatabase.h` | `UserDatabase` class — in-memory user store backed by a flat file; `authenticate()`, `addUser()`, `removeUser()`, `changePassword()`, `loadFromFile()`, `saveToFile()` |
| `gui/LoginWindow.h` | Login window declaration — `UserDatabase` member, UI field declarations, `portalAccessGranted` signal |
| `gui/LoginWindow.cpp` | Login window implementation — UI layout, stylesheet, credential validation, error display, `portalAccessGranted` signal emission |
| `gui/Dashboard.cpp` *(Admin & security sections)* | `buildAdminPage()`, `reloadUsersGrid()`, `onAddUser()`, `onRemoveUser()`, `onChangePassword()`, `onNavAdmin()`, `onNavLogout()`, role-based button enable/disable logic throughout `buildLayout()` and `buildSidebar()` |

### Key OOP Concepts Applied
- **Abstraction**: `User` is an abstract base class; it defines the `getSystemAccessRights()` contract without implementing it.
- **Inheritance**: `AdminUser`, `HRUser`, and `ViewerUser` each inherit from `User` and provide their own access-rights string.
- **Polymorphism (Runtime)**: `UserDatabase` stores `std::unique_ptr<User>` objects and calls the overridden `getSystemAccessRights()` polymorphically on whichever subtype is loaded.
- **Encapsulation**: Password hashes, roles, and session data are private to `User`; only controlled methods expose authentication logic.

### Core Features Delivered
- Three role tiers: **Admin** (full access), **HR** (read/write/attendance/payroll), **Viewer** (read-only)
- `authenticate()` validates username + password and returns the role string; empty string on failure
- User accounts persisted to `ems_users.txt` — auto-seeded with three default accounts on first run
- Login window with styled card, error label, and Enter-key support
- `portalAccessGranted(role, username)` signal wires login → dashboard launch in `main.cpp`
- Dashboard dynamically hides or disables buttons based on `liveSessionRole` at startup
- Admin panel: add new user (with role selection), remove user, change password — all persisted immediately
- Session displays the current user's name and role in the sidebar

---

## 📊 Contribution Summary Table

| Contributor | Module | Source Files | Lines of Code (approx.) |
|-------------|--------|-------------|--------------------------|
| Talha Sajjad | Employee Directory | `Employee.h/cpp`, `Database.h/cpp`, Dashboard (employees section) | ~2,500 |
| Habiba Farid | Biometric & Attendance | `FingerprintDialog.h/cpp`, `FaceIdDialog.h/cpp` | ~1,500 |
| Aina Bakhtawer | Payroll & Reports | `Manager.h`, payroll in `Employee.cpp`, Dashboard (reports section) | ~1,800 |
| Syeda Kashaf Fatima | Role-Based Security | `User.h`, `UserDatabase.h`, `LoginWindow.h/cpp`, Dashboard (admin/auth sections) | ~1,200 |

---

*Last updated: May 2026*
