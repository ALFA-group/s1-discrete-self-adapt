# Official Paper Repo for "Discrete Self-Adaptation in Competitive Coevolution for Constrained Hardware"

## Abstract
Self-adaptive competitive coevolutionary algorithms (CCEAs) typically rely on real-valued evolutionary parameters such as mutation rate. Yet emerging deployment targets, including specialized and resource-constrained hardware, often require integer-only or low-precision arithmetic which raises questions about how self-adaptation behaves under discrete constraints.

We investigate integer-only self-adaptive CCEAs, focusing on mutation rate and a Step Distribution Threshold (SDT) in an attacker–defender game, with results validated across Python and specialized-hardware implementations. Integer-valued parameters exhibit stable and effective adaptation, achieving convergence and performance comparable to continuous methods. Joint adaptation of mutation rate and SDT outperforms partially adaptive or fixed setups, maintaining a robust balance between exploration and exploitation across budget regimes.

We further show that integer-only self-adaptation responds reliably to environmental changes and extended evolutionary runs, and that these dynamics transfer to a pursuit–evasion style game. These findings establish discrete self-adaptation as a viable, generalizable mechanism for competitive coevolution on constrained hardware platforms.

## Requirements
* **Python 3.12** or newer
* **Dependencies**: Install the required packages using:
  ```bash
  pip install -r requirements.txt
    ```
* **Git LFS**: To be able to get the S1 data (`paper_runs.zip` <- 1.5 GB), you must install Git LFS (before cloning) using:
  ```bash
  git lfs install
    ```
* **Storage**: 10 GB of disk storage for the experimental data.

## Running the Code
1. Unzip `paper_runs.zip` into the project root. This contains our experimental S1 data for the plots. The unzipped data requires approximately **7 GB** of available disk space.

2. To generate the necessary Python data for the plots, run the experiment script using: `python pythonCode/experiments.py`. This will take a while to run.

3. Once the experiment script finishes, you can run all cells in `paper_plots.ipynb` to generate the paper plots. All figures will be exported to `paper_runs/plots/`. The final code block is computationally intensive and will take a few minutes to finish executing.
