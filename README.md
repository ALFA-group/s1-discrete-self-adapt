Official Paper Repo for "Discrete Self-Adaptation in Competitive Coevolution for Constrained Hardware"

Abstract:
Self-adaptive competitive coevolutionary algorithms (CCEAs) typically rely on real-valued evolutionary parameters such as mutation rate. Yet emerging deployment targets, including specialized and resource-constrained hardware, often require integer-only or low-precision arithmetic which raises questions about how self-adaptation behaves under discrete constraints.

We investigate integer-only self-adaptive CCEAs, focusing on mutation rate and a Step Distribution Threshold (SDT) in an attacker–defender game, with results validated across Python and specialized-hardware implementations. Integer-valued parameters exhibit stable and effective adaptation, achieving convergence and performance comparable to continuous methods. Joint adaptation of mutation rate and SDT outperforms partially adaptive or fixed setups, maintaining a robust balance between exploration and exploitation across budget regimes.

We further show that integer-only self-adaptation responds reliably to environmental changes and extended evolutionary runs, and that these dynamics transfer to a pursuit–evasion style game. These findings establish discrete self-adaptation as a viable, generalizable mechanism for competitive coevolution on constrained hardware platforms.
