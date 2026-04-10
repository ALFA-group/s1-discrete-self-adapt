from utils import *
from defendit import DefendItGame, DefendItGameConfig

@dataclass(kw_only=True)
class DefendItNonSpatialConfig(DefendItGameConfig):
    """Configuration for Non-Spatial (Tournament) DefendIt."""
    # Non-Spatial Specifics
    pop_size: int
    A: float
    min_param: float 

    rows: int = None
    cols: int = None

    inc_threshold: int = None

    default_sdt: int = 0
    prob_large_dist_step: int = None
    small_dist_max: int = None
    large_dist_max: int = None
    adapt_sdt: bool = None
    max_param: float = float("inf")

    def __post_init__(self):
        self.indices = np.arange(self.pop_size)
        self.neighbors = None 
        self.num_players = 2

    @property
    def generic_shape(self) -> Tuple[int, int]:
        return (self.num_players, self.pop_size)
    
    @property
    def genome_shape(self) -> Tuple[int, int, int]:
        return (self.num_rounds, *self.generic_shape)

    @property
    def resource_mask(self) -> int:
        return make_mask(self.num_resources)
    
class DefendItNonSpatialGame(DefendItGame):
    def __init__(self, config: DefendItNonSpatialConfig, files: Dict[str, TextIO]):
        
        super().__init__(config, files)
        self.powers_of_2 = (1 << np.arange(config.num_resources, dtype=np.uint16))
        
        # Initialize Float-based Mutation Threshold
        if config.default_mut_thresh < 0:
            self.mut_thresh = self.rng.random(config.generic_shape, dtype=np.float32)
            np.clip(self.mut_thresh, config.min_param, None, out=self.mut_thresh)
        else:
            self.mut_thresh = np.full(
                config.generic_shape, config.default_mut_thresh, dtype=np.float32
            )

    def play_against_self(self, move_budget: npt.NDArray[np.uint16],
                          selected_indices: npt.NDArray[np.uint16]) -> npt.NDArray[np.int16]:
        
        metrics = np.zeros((3, *self.config.generic_shape), dtype=np.uint16)
        metrics[self.IDX_BUDGET] = move_budget
        score = np.zeros(self.config.generic_shape, dtype=np.int16)
        curr_res_owners = self.config.resource_mask

        for i in range(self.config.num_rounds): 
            metrics[self.IDX_MOVE, ATTACKER] = self.ape_get(self.genome[i, ATTACKER], selected_indices[0])
            metrics[self.IDX_MOVE, DEFENDER] = self.ape_get(self.genome[i, DEFENDER], selected_indices[0])
            curr_res_owners = self._play_move(metrics, curr_res_owners, score)

        self._compute_final_score(metrics, score)
        return score

    def play_against_neighbor(self, selected_indices: npt.NDArray[np.int32], 
                              play_as_defender: bool, 
                              move_budget: npt.NDArray[np.uint16]) -> npt.NDArray[np.int16]:

        metrics = np.zeros((3, *self.config.generic_shape), dtype=np.uint16)
        metrics[self.IDX_BUDGET] = move_budget
        score = np.zeros(self.config.generic_shape, dtype=np.int16)
        curr_res_owners = self.config.resource_mask

        for i in range(self.config.num_rounds):
            if play_as_defender:
                # 0 is Defender, 1 is Attacker
                metrics[self.IDX_MOVE, ATTACKER] = self.ape_get(self.genome[i, ATTACKER], selected_indices[1])
                metrics[self.IDX_MOVE, DEFENDER] = self.ape_get(self.genome[i, DEFENDER], selected_indices[0])
            else:
                # 0 is Attacker, 1 is Defender
                metrics[self.IDX_MOVE, ATTACKER] = self.ape_get(self.genome[i, ATTACKER], selected_indices[0])
                metrics[self.IDX_MOVE, DEFENDER] = self.ape_get(self.genome[i, DEFENDER], selected_indices[1])

            curr_res_owners = self._play_move(metrics, curr_res_owners, score)

        self._compute_final_score(metrics, score)
        return score
    
    def selection(self, game_param):
        return self._tournament_selection(game_param)

    def _tournament_selection(self, cur_budget) -> Tuple[npt.NDArray[np.int16], npt.NDArray[np.int32]]:
        """Randomly selects 2 candidates for every slot in the population."""
        
        selected = self.rng.integers(0, self.config.pop_size, self.config.generic_shape)

        a1d1_payoffs = self.play_against_self(cur_budget, selected)
        a1d2_payoffs = self.play_against_neighbor(selected, False, cur_budget)
        a2d1_payoffs = self.play_against_neighbor(selected, True, cur_budget)

        dominated_mask = (a1d1_payoffs[DEFENDER] <= a1d2_payoffs[DEFENDER]) & \
                         (a1d1_payoffs[ATTACKER] <= a2d1_payoffs[ATTACKER])
        best_idx = np.where(dominated_mask, selected[1], selected[0])
        return a1d1_payoffs, best_idx
    
    def mutate_mut_thresh(self) -> None:
        """Floating point multiplicative mutation."""
        
        rand_thresh = self.rng.random(self.config.generic_shape)
        should_increase = rand_thresh <= self.config.inc_threshold
        
        increased = np.minimum(self.config.A * self.mut_thresh, 
                               self.config.num_resources * self.config.num_rounds)
        decreased = np.maximum(self.mut_thresh / self.config.A, self.config.min_param)
        
        self.mut_thresh = np.where(should_increase, increased, decreased)

    def mutate_genome(self) -> None:
        """Bitwise mutation based on float threshold."""

        scaled_thresh = self.mut_thresh / (self.config.num_resources * self.config.num_rounds)
        
        rand_thresh = self.rng.random(
            (self.config.num_resources, *self.config.genome_shape)
        )
        should_flip = rand_thresh <= scaled_thresh
        
        flip_mask = np.tensordot(self.powers_of_2, should_flip, axes=(0, 0)).astype(self.genome.dtype)
        self.genome ^= flip_mask

def game_kernel(files: Dict[str, TextIO], raw_config: Dict[str, Any],timing: bool = False):
    config = DefendItNonSpatialConfig(**raw_config)
    DefendItNonSpatialGame(config, files).run(timing=timing)