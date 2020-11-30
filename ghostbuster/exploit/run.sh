#!/bin/bash

set -e

./attacker/monitor |& ./attacker/reconstruct.py
