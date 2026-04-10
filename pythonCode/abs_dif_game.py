from utils import *

@dataclass(kw_only=True)
class AbsDifGameConfig(GenericConfig):
    """Absolute Difference Game Configuration."""

    winning_value: int
    losing_value: int
    difs: List[int]
    
class AbsDifGame(GenericGame):
    def __init__(self, config: AbsDifGameConfig, files: Dict[str, TextIO]):

        super().__init__(config, files)
        self.powers_of_2 = generate_powers_of_2(config.max_param.bit_length(), 5)
        
        # Correct the genome for this game
        self.genome = self.rng.integers(
            config.min_param, config.max_param, config.genome_shape, dtype=np.uint16
        )

    def _generate_number_by_bits(self, num_to_generate: int) -> npt.NDArray[np.uint16]:
        """Generates numbers by simulating bitwise mutations based on bias."""
        
        bit_len = self.config.max_param.bit_length()
        
        rand_thresh = self.rng.integers(
            self.config.min_param, 
            self.config.max_param, 
            (bit_len, num_to_generate, *self.config.generic_shape), 
            dtype=np.uint16
        )
        
        should_flip = rand_thresh <= self.mut_thresh
        rand_val = np.sum(should_flip * self.powers_of_2, axis=0, dtype=np.uint16)
        return np.clip(rand_val, self.config.min_param, self.config.max_param)

    def _calculate_scores(self, attacker: npt.NDArray, defender: npt.NDArray, 
                          allowed_dif: npt.NDArray) -> npt.NDArray[np.int16]:
        """Shared logic for calculating scores based on diffs."""

        diffs = np.abs(attacker.astype(np.int32) - defender.astype(np.int32))
        lost = diffs <= allowed_dif
        atk_scores = np.where(lost, self.config.losing_value, self.config.winning_value)
        def_scores = np.where(lost, self.config.winning_value, self.config.losing_value) 
        
        score = np.zeros(self.config.generic_shape, dtype=np.int32)
        score[ATTACKER] = np.sum(atk_scores, axis=0, dtype=np.int32)
        score[DEFENDER] = np.sum(def_scores, axis=0, dtype=np.int32)
        return score

    def selection(self, game_param):

        return self._spatial_pareto_dom_selection(game_param)

    def play_against_self(self, allowed_dif: npt.NDArray[np.uint16]) -> npt.NDArray[np.int16]:
        
        return self._calculate_scores(
            self.genome[:, ATTACKER], self.genome[:, DEFENDER], allowed_dif
        )

    def play_against_neighbor(self, selected_neighbor: npt.NDArray[np.int32], 
                              play_as_defender: bool, 
                              allowed_dif: npt.NDArray[np.uint16]) -> npt.NDArray[np.int16]:

        if play_as_defender:
            attacker = self.ape_get(self.genome[:, ATTACKER], selected_neighbor)
            defender = self.genome[:, DEFENDER]
        else:
            attacker = self.genome[:, ATTACKER]
            defender = self.ape_get(self.genome[:, DEFENDER], selected_neighbor)

        return self._calculate_scores(attacker, defender, allowed_dif)

    def mutate_genome(self) -> None:

        new_vals = self._generate_number_by_bits(self.config.num_rounds)
        rand_thresh = self.rng.integers(0, MAX_VAL, self.config.genome_shape)
        
        should_change = rand_thresh <= self.mut_thresh
        self.genome = np.where(should_change, new_vals, self.genome)

    def get_cur_adaptive_param(self, round_i: int):

        round_i = min(max(0, int(round_i)), len(self.config.difs)-1)
        return self.config.difs[round_i]
    
def game_kernel(files: Dict[str, TextIO], raw_config: Dict[str, Any],  timing: bool = False):
    config = AbsDifGameConfig(**raw_config)
    AbsDifGame(config, files).run(timing=timing)

