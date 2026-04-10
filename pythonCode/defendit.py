from utils import *

@dataclass(kw_only=True)
class DefendItGameConfig(GenericConfig):
    """Configuration specific to the Resource Defense Game."""
    num_resources: int

    res_value: int
    res_cost: int

    atk_budgets: List[int]
    def_budgets: List[int]

    @property
    def resource_mask(self) -> int:
        return make_mask(self.num_resources)
    
class DefendItGame(GenericGame):
    # Metric Indices
    IDX_COST = 0
    IDX_MOVE = 1
    IDX_BUDGET = 2

    def __init__(self, config: DefendItGameConfig, files: Dict[str, TextIO]):

        super().__init__(config, files)
        self.powers_of_2 = (1 << np.arange(config.num_resources, dtype=np.uint16))

    def _compute_final_score(self, metrics: npt.NDArray[np.uint16], 
                             scores: npt.NDArray[np.int16]) -> None:
        """Applies resource values and penalizes over-budget players."""

        raw_score = (scores.astype(np.int32) * self.config.res_value) & WORD_MASK
        is_overbudget = metrics[self.IDX_COST] > metrics[self.IDX_BUDGET]
        scores[:] = np.where(is_overbudget, 
                             metrics[self.IDX_COST].astype(np.int16) * -1, 
                             raw_score)

    def _play_move(self, metrics: npt.NDArray[np.uint16], 
                   curr_res_owners: int | npt.NDArray[np.uint16],
                   scores: npt.NDArray[np.int16]) -> npt.NDArray[np.uint16]:
        """Executes a single timestep logic."""
        
        # Check if over budget and zero out moves if so
        num_ones = count_bits(metrics[self.IDX_MOVE])
        metrics[self.IDX_COST] += num_ones
        is_over_budget = metrics[self.IDX_COST] > metrics[self.IDX_BUDGET]
        metrics[self.IDX_MOVE] = np.where(is_over_budget, 0, metrics[self.IDX_MOVE])

        # Find resources where only attacker or only defender is attempting to capture
        atk_captures = metrics[self.IDX_MOVE, ATTACKER] & bit_not(
            metrics[self.IDX_MOVE, DEFENDER], self.config.resource_mask
        )
        def_captures = metrics[self.IDX_MOVE, DEFENDER] & bit_not(
            metrics[self.IDX_MOVE, ATTACKER], self.config.resource_mask
        )

        # Set bits where attacker captures to 0 and defender captures to 1 
        curr_res_owners = (curr_res_owners & bit_not(
            atk_captures, self.config.resource_mask
        )) | def_captures

        num_def_owned = count_bits(curr_res_owners).astype(np.int16)
        scores[ATTACKER] += (self.config.num_resources - num_def_owned)
        scores[DEFENDER] += num_def_owned

        return curr_res_owners

    def selection(self, game_param):

        return self._spatial_pareto_dom_selection(game_param)
    
    def play_against_self(self, move_budget: npt.NDArray[np.uint16]) -> npt.NDArray[np.int16]:

        metrics = np.zeros((3, *self.config.generic_shape), dtype=np.uint16)
        metrics[self.IDX_BUDGET] = move_budget
        score = np.zeros(self.config.generic_shape, dtype=np.int16)
        curr_res_owners = self.config.resource_mask # Initially all defender owned

        for i in range(self.config.num_rounds): 
            metrics[self.IDX_MOVE] = self.genome[i]
            curr_res_owners = self._play_move(metrics, curr_res_owners, score)

        self._compute_final_score(metrics, score)
        return score

    def play_against_neighbor(self, selected_neighbor: npt.NDArray[np.int32], 
                              play_as_defender: bool, 
                              move_budget: npt.NDArray[np.uint16]) -> npt.NDArray[np.int16]:

        metrics = np.zeros((3, *self.config.generic_shape), dtype=np.uint16)
        metrics[self.IDX_BUDGET] = move_budget
        score = np.zeros(self.config.generic_shape, dtype=np.int16)
        curr_res_owners = self.config.resource_mask # Initially all defender owned

        for i in range(self.config.num_rounds):
            if play_as_defender:
                metrics[self.IDX_MOVE, ATTACKER] = self.ape_get(self.genome[i, ATTACKER], selected_neighbor)
                metrics[self.IDX_MOVE, DEFENDER] = self.genome[i, DEFENDER]
            else:
                metrics[self.IDX_MOVE, ATTACKER] = self.genome[i, ATTACKER]
                metrics[self.IDX_MOVE, DEFENDER] = self.ape_get(self.genome[i, DEFENDER], selected_neighbor)

            curr_res_owners = self._play_move(metrics, curr_res_owners, score)

        self._compute_final_score(metrics, score)
        return score

    def mutate_genome(self) -> None:

        rand_thresh = self.rng.integers(
            0, MAX_VAL + 1, (self.config.num_resources, *self.config.genome_shape)
        )
        should_flip = rand_thresh <= self.mut_thresh
        flip_mask = np.tensordot(self.powers_of_2, should_flip, axes=(0, 0)).astype(self.genome.dtype)
        self.genome ^= flip_mask

    def get_cur_adaptive_param(self, round_i: int):

        atk_round_i = min(max(0, int(round_i)), len(self.config.atk_budgets)-1)
        def_round_i = min(max(0, int(round_i)), len(self.config.def_budgets)-1)

        result = np.zeros(self.config.generic_shape, dtype=np.uint16)
        result[ATTACKER] = self.config.atk_budgets[atk_round_i] // self.config.res_cost
        result[DEFENDER] = self.config.def_budgets[def_round_i] // self.config.res_cost

        return result
        
def game_kernel(files: Dict[str, TextIO], raw_config: Dict[str, Any], timing: bool = False):
    config = DefendItGameConfig(**raw_config)
    DefendItGame(config, files).run(timing=timing)
