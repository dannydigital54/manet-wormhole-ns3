# MANET Wormhole NS-3

## Description

This repository contains a reproducible NS-3 simulation for a MANET wormhole attack scenario.

## Parameters

* 30 nodes
* Area: 80 × 80
* AODV routing
* 5 UDP flows at 256 Kbps

## Execution (Ubuntu / Linux)

1. Install NS-3 (version 3.46 or compatible)

2. Copy the file into:
   ns-3/scratch/

3. Build and run:

   ./ns3 build
   ./ns3 --run scratch/manet-wormhole-exp

## Notes

This scenario is illustrative and supports a research study.

## Reproducibility Statement

This repository provides the implementation details required to reproduce the simulation scenario described in the associated research work on MANET security and wormhole attacks.
