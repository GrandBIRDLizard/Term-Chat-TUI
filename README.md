# Term-Chat-TUI

Term-Chat-TUI is an encrypted terminal-based chat application written in C. It provides a simple text user interface (TUI) for real-time communication directly from your terminal.
---
## Features

- Lightweight and fast terminal chat interface  
- Written in C for performance
- Data streams encrypted with SSL  
- Easy to build with Makefile  
- Cross-platform support (Linux, macOS)
---
## Getting Started

### Prerequisites

- C compiler (e.g., gcc, clang)
- make utility
---
### Building

Clone the repository and build the project:

```sh
git clone https://github.com/GrandBIRDLizard/Term-Chat-TUI.git
cd Term-Chat-TUI
make
make certs // follow instructions to set up SSL certification and keys on server
```
---
### Running

After building, run the executable:
For running your own server for other clients to connect to

```sh
./server <PORT_NUMBER> 
```
For running the client to connect to a server 

```sh
./client
```
---
## Usage
- Launch the application in your terminal.
- Follow the on-screen instructions to join or create a chat. 
[Example Image](term-chat-menu.png)
  
- Chat
[Example Image](chat-server.png)

- Voice Chat
[Example Image](audio-serv.png)

---
## Contributing

Contributions are welcome! Please open an issue or submit a pull request.

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.
---
