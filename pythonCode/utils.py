from dataclasses import dataclass, field

import numpy as np
import numpy.typing as npt

from typing import Tuple, Dict, Any, TextIO, List

ATTACKER = 0
DEFENDER = 1

# Board Constraints
MAX_WORD_USAGE = 450
WORD_SIZE = 16
MAX_VAL = 2 ** WORD_SIZE - 1
WORD_MASK = (1 << WORD_SIZE) - 1

def make_mask(num_bits: int) -> int:
    return (1 << num_bits) - 1

def bit_not(n: int, mask: int) -> int:
    return (~n) & mask

BIT_COUNT_TABLE = np.array([i.bit_count() for i in range(MAX_VAL+1)], dtype=np.uint16)

def count_bits(arr: npt.NDArray[np.uint16]) -> npt.NDArray[np.uint16]:
    return BIT_COUNT_TABLE[arr]

def generate_powers_of_2(num_bits: int, num_dims: int) -> npt.NDArray[np.uint16]:
    """Generates powers of 2 broadcastable over specified dimensions."""

    powers = (1 << np.arange(num_bits, dtype=np.uint16))[:, None, None, None]
    target_shape = (num_bits,) + (1,) * (num_dims - 1) # (num_bits, 1, 1...) based on num_dims
    return powers.reshape(target_shape)

@dataclass(kw_only=True)
class GenericConfig:
    """Base configuration shared across different game types."""
    rows: int
    cols: int

    num_rounds: int

    inc_threshold: int

    min_param: int
    max_param: int
    default_mut_thresh: int
    adapt_mut_thresh: bool

    default_sdt: int
    prob_large_dist_step: int
    small_dist_max: int
    large_dist_max: int
    adapt_sdt: bool

    seed: int
    generations: int
    round_param_change_interval: int
    checkpoint_interval: int

    num_players: int = 2

    indices: npt.NDArray[np.int32] = field(init=False)
    neighbors: npt.NDArray[np.int32] = field(init=False)

    def __post_init__(self):
        pop_size = self.rows * self.cols
        self.indices = np.arange(pop_size).reshape((self.rows, self.cols))

        neighbors = np.full((4, self.rows, self.cols), pop_size, dtype=np.int32)
        neighbors[0, 1:, :] = self.indices[:-1, :] # North
        neighbors[1, :-1, :] = self.indices[1:, :] # South
        neighbors[2, :, 1:] = self.indices[:, :-1] # West
        neighbors[3, :, :-1] = self.indices[:, 1:] # East
        
        neighbors.sort(axis=0) 
        neighbors[neighbors == pop_size] = -1
        self.neighbors = neighbors

    @property
    def generic_shape(self) -> Tuple[int, int, int]:
        return (self.num_players, self.rows, self.cols)

    @property
    def genome_shape(self) -> Tuple[int, int, int, int]:
        return (self.num_rounds, *self.generic_shape)

class GenericGame:
    def __init__(self, config: GenericConfig, files: Dict[str, TextIO]):
        self.config = config
        self.files = files
        self.rng = np.random.default_rng(config.seed)

        # Initial state
        self.genome = np.zeros(config.genome_shape, dtype=np.uint16)
        self.sdt = np.full(
            config.generic_shape, config.default_sdt, dtype=np.uint16
        )

        if config.default_mut_thresh < 0:
            self.mut_thresh = self.rng.integers(
                config.min_param, config.max_param + 1, config.generic_shape, dtype=np.uint16
            )
        else:
            self.mut_thresh = np.full(
                config.generic_shape, config.default_mut_thresh, dtype=np.uint16
            )

    def play_against_self(self, game_param: npt.NDArray[np.uint16]) -> npt.NDArray[np.int16]:
        """
        Simulates one round of a competition between the attacker and defender 
        on the same spot.
        """
        raise NotImplementedError("Child class must implement play_against_self")

    def play_against_neighbor(self, selected_neighbor: npt.NDArray[np.int32], 
                              play_as_defender: bool, 
                              game_param: npt.NDArray[np.uint16]) -> npt.NDArray[np.int16]:
        """
        Simulates one round of a competition between the attacker and defender 
        on pre-selected neighboring spots.
        """
        raise NotImplementedError("Child class must implement play_against_neighbor")

    def mutate_sdt(self) -> None:
        """Evolves the probability of using a large mutation step."""
 
        res = self.sdt.astype(np.int32)
        step = self.config.prob_large_dist_step

        rand_thresh = self.rng.integers(0, MAX_VAL + 1, self.config.generic_shape)
        should_increase = rand_thresh <= self.config.inc_threshold
        res += np.where(should_increase, step, -step)

        np.clip(res, self.config.min_param, self.config.max_param, out=res)
        self.sdt = res.astype(self.sdt.dtype)

    def mutate_mut_thresh(self) -> None:
        """Evolves the mutation threshold."""

        res = self.mut_thresh.astype(np.int32)

        small_dist = self.rng.integers(0, self.config.small_dist_max + 1, self.config.generic_shape, np.int32)
        large_dist = self.rng.integers(0, self.config.large_dist_max + 1, self.config.generic_shape, np.int32)
        
        rand_thresh = self.rng.integers(0, MAX_VAL + 1, self.config.generic_shape)
        use_large_dist = rand_thresh <= self.sdt
        step = np.where(use_large_dist, large_dist, small_dist)

        rand_thresh = self.rng.integers(0, MAX_VAL + 1, self.config.generic_shape)
        should_increase = rand_thresh <= self.config.inc_threshold
        res += np.where(should_increase, step, -step)
        
        np.clip(res, self.config.min_param, self.config.max_param, out=res)
        self.mut_thresh = res.astype(self.mut_thresh.dtype)

    def sample_random_neighbor(self) -> npt.NDArray[np.int32]:
        """Selects a valid neighbor for every cell in the grid."""

        num_valid_neighbors = np.sum(self.config.neighbors != -1, axis=0)
        rand_choice_i = self.rng.integers(0, num_valid_neighbors)
        selected = np.take_along_axis(self.config.neighbors, rand_choice_i[None, ...], axis=0)
        return selected.squeeze(0)
    
    def selection(self,
                  game_param: npt.NDArray[np.uint16]) -> Tuple[npt.NDArray[np.int16], npt.NDArray[np.int32]]:
        """Evaluate the population's performance and select the best-performing individuals."""
        raise NotImplementedError

    def _spatial_pareto_dom_selection(self, 
                                     game_param: npt.NDArray[np.uint16]) -> Tuple[npt.NDArray[np.int16], npt.NDArray[np.int32]]:
        """Core Co-Evolutionary Selection Algorithm."""
        
        selected_neighbor_i = self.sample_random_neighbor()

        a1d1_payoffs = self.play_against_self(game_param)
        a1d2_payoffs = self.play_against_neighbor(selected_neighbor_i, False, game_param)
        a2d1_payoffs = self.play_against_neighbor(selected_neighbor_i, True, game_param)
        
        dominated_mask = (a1d1_payoffs[DEFENDER] <= a1d2_payoffs[DEFENDER]) & \
                         (a1d1_payoffs[ATTACKER] <= a2d1_payoffs[ATTACKER])
        best_idx = np.where(dominated_mask, selected_neighbor_i, self.config.indices)
        return a1d1_payoffs, best_idx

    def replace_individuals(self, best_idx: npt.NDArray[np.int32],
                            to_copy: Tuple[npt.NDArray[np.uint16], ...]) -> None:
        """Overwrites individuals in the population with their superior neighbors."""

        for var in to_copy:
            for i in range(self.config.num_players):
                var[i] = self.ape_get(var[i], best_idx)

        flat_genome = self.genome.reshape(self.config.num_rounds, self.config.num_players, -1)
        self.genome = flat_genome[:, :, best_idx.flatten()].reshape(self.config.genome_shape)

    def run(self, timing: bool = False):
        """Executes the evolutionary simulation loop for a set number of generations."""

        gen_counter = 0
        checkpoint_counter = 0
        round_i = 0

        for _ in range(self.config.generations):
            if gen_counter == 0:
                round_param = self.get_cur_adaptive_param(round_i)

            # Selection
            payoff, best_idx = self.selection(round_param)
            
            # Replacement
            self.replace_individuals(best_idx, (payoff, self.mut_thresh, self.sdt))

            if not timing:
                if checkpoint_counter == 0:
                    self.write_checkpoint(payoff)
                
                checkpoint_counter += 1
                if checkpoint_counter == self.config.checkpoint_interval:
                    checkpoint_counter = 0

            # Mutation
            if self.config.adapt_sdt:
                self.mutate_sdt()

            if self.config.adapt_mut_thresh:
                self.mutate_mut_thresh()

            self.mutate_genome()

            gen_counter += 1
            if gen_counter == self.config.round_param_change_interval:
                round_i += 1
                gen_counter = 0

        payoff, _ = self.selection(round_param)

        if not timing:
            self.write_checkpoint(payoff)

    def write_checkpoint(self, payoff: npt.NDArray):
        """Writing data to files."""

        self.write_all_data_to_files(payoff, (self.files['atk_payoff'], self.files['def_payoff']))
        self.write_all_data_to_files(self.mut_thresh, (self.files['atk_mutRate'], self.files['def_mutRate']))
        
        if "atk_mutSdt" in self.files:
            self.write_all_data_to_files(self.sdt, (self.files['atk_mutSdt'], self.files['def_mutSdt']))

    @staticmethod
    def ape_get(registers: npt.NDArray[Any], neighbor_i: npt.NDArray[np.int32]) -> npt.NDArray[Any]:
        """Vectorized gather operation to get values from neighbors."""
        
        return registers.ravel()[neighbor_i]
    
    def get_cur_adaptive_param(round_i: int):
        """Helper to fetch game parameters (like budgets and difs) from config."""
        raise NotImplementedError
    
    def mutate_genome(self):
        """Apply random modifications to the population's genomes."""
        raise NotImplementedError
    
    @staticmethod
    def write_all_data_to_files(data: npt.NDArray[Any], files: Tuple[TextIO, ...]) -> None:
        """Writes data flattened to CSV-like format."""

        for i, f in enumerate(files):
            f.write(", ".join(map(str, data[i].flatten().tolist())))
            f.write("\n")