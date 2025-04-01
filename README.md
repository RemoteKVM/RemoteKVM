# RemoteKVM

![logo](https://gitlab.com/omerShva/images-for-readme/-/raw/main/RemoteKVM_big_logo.png)

## Description
**RemoteKVM** is a project developed as part of a 12th-grade assignment at "Magshimim Cyber" by Liron Fridman and Omer Shvartzvald. The project uses the **KVM API** to create a hypervisor that allows remote SSH-based connections for managing virtual machines.

## Links
- **Website:** [https://remotekvm.online](https://remotekvm.online)
- **Project Documentation:** [View Documentation](https://docs.google.com/document/d/1aULOYGXbrKZjQeXdZRdgJqwQUQY-bvm8ffIVtNzeAHA/edit?usp=sharing)
- **PowerPoint Presentation:** [View PowerPoint](https://docs.google.com/presentation/d/1yuHf-qhQIX3mmBx9EBr3icslYw3DcNa-/edit?usp=sharing&ouid=106854554423523955510&rtpof=true&sd=true)

## Project Structure
This repository contains the following directories:

- **`hypervisor/`**: C code for the hypervisor (runs on Linux)
- **`ssh-server/`**: C++ code for the self-implemented SSH2 server (runs on Linux). This server runs the hypervisor as a subprocess.
- **`https-server/`**: Node.js + React web server for the official website ([https://remotekvm.online](https://remotekvm.online))
- **`wss-ssh-tunnel/`**: Node.js tunnel for the web terminal (must be run on the same accessible server as the https-server)
- **`frp-configuration/`**: Configuration files for [frp](https://github.com/fatedier/frp). These files include configurations for the client (laptop running the `ssh-server`) and the server (running both `https-server` and `wss-ssh-tunnel`).

## Network Diagram
![Network Diagram](https://gitlab.com/omerShva/images-for-readme/-/raw/main/web_diagram.png)

## Setup

We use an **Azure MySQL service**, so connection details are required for proper setup.  

### Required Environment Variables

The following environment variables need to be set for each server:

### 1. `https-server`
Set the following environment variables for the web server:  
- `DB_HOST` – Hostname of the MySQL database  
- `DB_USER` – MySQL username  
- `DB_PASSWORD` – MySQL password  
- `DB_NAME` – Name of the MySQL database  
- `DB_PORT` – MySQL port (default: `3306`)  
- `DB_SSL` – Boolean value (`true/false`) indicating whether SSL is required for database connection  
- `JWT_SECRET` – Secret key used for JWT authentication  
- `JWT_EXPIRATION` – JWT expiration time (e.g., `1d`)  
- `SSH_SERVER_SECRET_KEY` – Shared secret key that allows the SSH server to modify VM status  

### 2. `ssh-server`
The following environment variable is required for the SSH server:  
- `SSH_SERVER_SECRET_KEY` – Must match the value set in the `https-server`  

---

