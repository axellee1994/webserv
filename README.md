# webserv

## Description

**webserv** is a lightweight, high-performance HTTP server written in C++. It is designed to support multiple simultaneous connections, configurable routing, CGI execution, and static file serving. The project aims to provide a modular, standards-compliant, and educational web server suitable for learning, experimentation, and lightweight deployment.

### Main Features
- HTTP/1.1 compliant
- Configurable via external configuration files
- Static and dynamic file serving (CGI support)
- Graceful shutdown and signal handling
- Modular, maintainable C++98 codebase

---

## Installation Instructions

### Prerequisites
- **C++ Compiler:** clang++-12 (or compatible)
- **Make:** GNU Make
- **POSIX environment:** Linux or macOS recommended

### Steps
```bash
git clone https://github.com/axellee1994/webserv.git
cd webserv
make
```
This will generate the `webserv` executable in the project root.

---

## Usage Instructions

Run the server with a configuration file:
```bash
./webserv config4.0.conf
```

- The configuration file specifies server blocks, routes, error pages, and CGI options.
- Example configuration files are provided in the repository.

### Example: Accessing the Server
1. Start the server as above.
2. In your browser, navigate to `http://localhost:8080` (or the port specified in your config).
3. Static files are served from the `www/` directory by default.

---

## Contribution Guidelines

We welcome contributions! Please follow these steps:

1. **Fork** the repository on GitHub.
2. **Clone** your fork locally and create a new branch for your feature or bugfix.
3. Ensure your code follows the project's C++98 style and includes clear comments.
4. Add tests or examples if applicable.
5. Submit a **pull request** with a clear description of your changes.

**Code Style:**
- Follow existing formatting and naming conventions.
- Document complex logic and public interfaces.
- Keep commits focused and descriptive.

---

## License Information

This project is licensed under the MIT License. You are free to use, modify, and distribute this software. See the LICENSE file for details.

---

## Contact Information

For questions, suggestions, or support, please contact:
- **GitHub:** [axellee1994](https://github.com/axellee1994)
- **Email:** [your-email@example.com]

---

Thank you for using and contributing to **webserv**! If you find this project helpful, please consider starring the repository or submitting improvements.