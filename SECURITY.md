# Security Policy

## Supported versions

| Version | Supported |
|---------|-----------|
| 1.0.x   | ✅        |
| < 1.0   | ❌        |

## Reporting a vulnerability

This is a local desktop application that reads audio loopback data in memory only — it does not open network sockets, write files, or execute downloaded code. The attack surface is small, but if you find something:

1. **Do not open a public issue** for exploitable vulnerabilities.
2. Use GitHub's [private vulnerability reporting](https://github.com/lijunjie2232/audio_device_spector/security/advisories/new) on this repository.
3. Include a description, reproduction steps, and affected version/commit.

You can expect an initial response within 7 days. Fixes will be released as soon as practical, and you will be credited in the changelog unless you prefer otherwise.

## Scope

In scope:
- Memory-safety issues in capture/render paths
- Privilege or sandbox boundary issues
- Anything that would let a remote or unprivileged local actor affect the process

Out of scope:
- Social engineering of users
- Issues requiring an already-compromised machine
