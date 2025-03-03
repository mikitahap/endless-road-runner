# Endless Road Runner in C

A **Crossy Road**-inspired console game developed in **C** as a first-semester university project.

## 📌 Overview

Crossy Road is a simple console-based game where the player controls a frog attempting to cross a series of roads, avoiding cars, trees, and a chasing stork. The game is built using **C** and utilizes the **PDCurses library** for console rendering.

## 🎮 Features

- **3-level gameplay**: Progress through 3 levels by reaching the end of each.
- **Obstacles**: Increasing difficulty over time, friendly and enemy cars.
- **Simple controls**: Use the keyboard to navigate.
- **Time tracking**: Saves high scores to a leaderboard.

## ⌨️ Controls

- **Arrow keys**: Move the player up, down, left, and right.
- **Esc**: Exit the game.

## 📂 Project Structure

```
endless-road-runner-c/
├── CrossyRoad.c            # Main game logic
├── CrossyRoad.vcxproj      # Visual Studio project file
├── CrossyRoad.vcxproj.filters # Project filters
├── config.txt              # Game settings
├── curses.h                # Header for PDCurses
├── game_state.txt          # Saves game progress
├── leaderboard.txt         # Stores high scores
├── pdcurses.lib            # PDCurses library file
├── README.md               # Project documentation
└── .gitignore              # Git ignored files
```

## 📜 License

This project is open-source, feel free to use the code.

