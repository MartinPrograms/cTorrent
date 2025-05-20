# cTorrent
A minimal BitTorrent client written in C, with loose components for easy integration into other projects.

## Overview
cTorrent is a simple BitTorrent CLI written in C. It is a very minimal implementation of the BitTorrent protocol, with a focus on simplicity and ease of understanding.

## Features
- KISS principle (Keep It Simple, Stupid)
- Works on POSIX systems (Linux, MacOS, etc.)
- Uses Check for unit testing
- MIT license
- As simple as possible

## Components
- /src/Application/ArgParser - Command line argument parser
- /src/Application/Settings - Configuration/Setting file parser
- /src/BitTorrent/CLI - Command line interface (cTorrent binary)  
- /src/Core/File - High level file abstraction
- /src/Core/Logger - Basic logging system
- /src/Core/Networking - Network abstraction layer
- /src/Core/Socket - Socket abstraction layer (network builds on top of this)
- /src/Core/Thread - Thread abstraction layer
- /src/Engine/ConnectionManager - Connection manager for the BitTorrent protocol
- /src/Engine/PeerManager - Peer manager for the BitTorrent protocol
- /src/Engine/PieceManager - Piece manager for the BitTorrent protocol
- /src/Protocol/BenCode - Bencoding and Bdecoding, for .torrent files
- /src/Protocol/BitTorrent - BitTorrent protocol implementation

## Building
To build cTorrent, you can run the build.sh script which runs cmake and make. This will create a build directory and run cmake in it, then run make to build the project. The resulting binary will be in the build/bin directory.

To test cTorrent, run test.sh which uses cmake to run the tests.

## Running
Building

A helper script automates the build process:
```bash
./build.sh
```
To compile and run tests:
```bash
./test.sh
```
Usage
```bash
ctorrent <path/to/torrentfile.torrent> [-dir <output_directory>]
```
`<torrentfile.torrent>` — Path to the .torrent file  
`-dir <output_directory>` — (Optional) Destination directory. Defaults to ./<torrent_name>/  
The client will create the target directory if it does not exist.