# Wize
Wize, a C++ UCI-compliant chess engine, integrates a neural network (NNUE) for accurate evaluation and employs a range of advanced search techniques like LazySMP and Multithreading within Alpha-Beta framework, Wize doesn't strive to be a top-tier engine but serves as a valuable resource for developers to enhance their own chess engines.

<p align="center">
  <img src="https://github.com/yanpuri/Wize/assets/121260820/b92ce538-2b81-4533-ada7-8bcf982f962d" alt="Wize Chess Engine">
  <br>
  <em><strong>Wize, Strong and Open Source Chess Engine Written in C++</strong></em>
</p>

## Features
* Architecture
  * Bitboard Representation

* Search 
  * Alpha-Beta Pruning
  * Negamax Framework
  * Transposition Tables
  * Principal Variation Search
  * Quiescence Search
  * Null-move Pruning
  * Internal Iterative Deepening
  * Aspiration Window
  * Repetition Detection
  * Killer Move, History
  * MVV-LVA Capture Ordering
  * LazySMP
  * Multithreading
  * Heuristic Moves
  * Polyglot Opening Book
  
* Evaluation
  * `NNUE Evaluation (Features=HalfKP(Friend)[41024->256x2],Network=AffineTransform[1<-32](ClippedReLU[32](AffineTransform[32<-32](ClippedReLU[32](AffineTransform[32<-512](InputSlice[512(0:512)]))))))`

## Improvements
  * Implement Syzygy Bases, SyzygyPath, ...
  * Improve the NN Structure
## Current ELO
  * **ELO: 2850-3000~** 

 ## Requirements
 * Wize software requires GCC(GNU Compiler Collection) or CLang, C++ version: CXX >= C++17 !
 * It is recommended that the terminal uses a TrueType Font (ttf), Consolas and other monospaced fonts are recommended for the best user experience [Unicode characters and ANSI escape code] (However, most engines are 
   linked directly to the GUI and don't really need to interact with the terminal).
 
 ## Installation/Usage
 Clone the repository:

```bash
$ git clone https://github.com/yanpuri/Wize.git
```
Then, in the directory:
```bash
$ cd Wize\Wize
$ make
$ Wize.exe
```
Run the generated .exe file in the install dir.
 
* How to work the engine?
The UCI (Universal Chess Interface) serves as a widely adopted protocol for interacting with a chess engine and is the preferred method for communication in graphical user interfaces (GUIs) and chess-related tools. The guidelines are outlined in the UCI protocol, which can be accessed and referenced at this link http://page.mi.fu-berlin.de/block/uci.htm

## Images

### Gameboard
![Gameboard](https://github.com/yanpuri/Wize/assets/121260820/0f71bdf5-ff01-4370-a158-a5095b6bb38d)

## Acknowledgements
* [BluefeverSoftware](https://www.youtube.com/@BlueFeverSoft), YouTube series about Vice engine.
* [CodeMonkeyKing](https://www.youtube.com/@chessprogramming591), General chess programming videos.
* Vice Programming Discord
* [ChessProgrammingWiki](https://www.chessprogramming.org/Main_Page).

## Support Me
If you find RepoUp useful, consider supporting me by:

- Starring the repository on GitHub
- Sharing the tool with others
- Providing feedback and suggestions
- Follow me for more :)
    
---
For any issues or feature requests, please open an issue on GitHub. Happy coding!
<center>
<div style="text-align: center;">
  <p align="center">
    <img src="https://github.com/user-attachments/assets/88a21822-dd10-4b3d-89d4-bad482f3a605" alt="octodance" width="100" height="100" style="margin-right: 10px;"/>
  </p>
</div>
</center>


