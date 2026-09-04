# Snake Cube Analysis in Three Dimensions

The original goal of this project was to analyse the solution space of the 4x4x4 snake cube. However after writing a program to analyse the 3x3x3 snake cube I realised just how unfeasible the original goal was. With some rough estimates my current program might be able to find all the solutions to a 4x4x4 snake cube in 443 years.

## Useful Resources

### [https://www.jaapsch.net/puzzles/snakecube.htm](https://www.jaapsch.net/puzzles/snakecube.htm)

Contains a table of number of solutions against number of straights, that I used to see if my code was working correctly. This is what my results table is based on, with one small change. It contains not only the total number of snakes, but also the total number of solutions.

### [https://github.com/ThoAppelsin/snake-cube-puzzle-variations](https://github.com/ThoAppelsin/snake-cube-puzzle-variations)

I used this a reference for the format of my generated solutions. Unfortunatley I bleive this project has a bug, were some of the reverse solutions of palindromic snakes are include again (I suspect this because I had a similar bug which caused the total number of solutions to 52055, which is the same in this project and is the wrong number).

## What is a Unique Solution

There are three transformations you can apply to a solution:

- Rotations
- Mirror images
- Reversing

Programmatically these can be implemented as:

- Mapping each axis to another unique axis (x, y, z) -> (y, x, z) \[Rotations\]
- Reversing each axis. (-, -, +) This is mirroring across x and y axis \[Mirror images\]
- Checking if the previous two can be used to transform one solution to the other when one solution is reversed \[Reversing\]

# Compilation

```
> make
```

# Results

The full table of results can be found at [results/3x3x3/report.md](https://github.com/FeliksWare/Snake-cube-analysis/tree/main/results/3x3x3/report.md) and the full list of solutions at [results/3x3x3/solutions.txt](https://github.com/FeliksWare/Snake-cube-analysis/tree/main/results/3x3x3/solutions.txt).

Some important values for the 3x3x3 snake cube that can help you, if you are undertaking a similar problem:

- Unique Hamiltonian paths (total number of solutions) - 51704
- Total number of solvable snakes - 11487
- Total number of snakes with unique solutions - 3658
- Maximum number of solutions a single snake has - 142
- Total number of solvable palindromic snakes - 77
- Total number of solutions to palindromic snakes - 413

# analyse3x3x3.c

Can only anaylse the 3x3x3 snake cube as the name would imply.

# analyseXYZ.c

Can analyse any cuboid who's volume is less then or equal 66 (I think this is a reasonable constraint considering the fact that the only cuboids that can reach this constraint are of the form Nx1x1).

To change which cuboid is being analysed you only have to change the three macros CUBE_WIDTH, CUBE_HEIGHT, CUBE_DEPTH (the names are in retrospec a bit stupid but) at the top of the program. CUBE_WIDTH has to be greater than 1 if you want any results as the first direction is hard coded to be positive x. I would also recomend a descending order for these parameters, because of the layout in memory (but I have not tested this my self).
