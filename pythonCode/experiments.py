import defendit, abs_dif_game, non_spatial_defendit
from utils import *
import json, os, time, yaml, random

DEFAULT_NUM_TRIALS = 10
DEFAULT_RAND_SEED = 123

games = {"defendit": defendit, 
         "abs_dif_game": abs_dif_game, 
         "non_spatial_defendit": non_spatial_defendit,
        }

def run_game(game_name, output_dir, config, timing = False):
    random.seed(DEFAULT_RAND_SEED)

    with open(f"{output_dir}/run_params.json", "w") as f:
        json.dump(config, f)

    game = games[game_name]

    print(f"running the experiment being saved to: {output_dir} for {DEFAULT_NUM_TRIALS} trials")
    
    if timing: start = time.time()

    for _ in range(DEFAULT_NUM_TRIALS):
        config['seed'] = random.randint(1, 5000)
        if timing:
            files = {}
        elif game_name == "non_spatial_defendit":
            files = {
                "atk_payoff": f"{output_dir}/attacker_payoffs_seed{config['seed']}.csv",
                "def_payoff": f"{output_dir}/defender_payoffs_seed{config['seed']}.csv",
                "atk_mutRate": f"{output_dir}/attacker_mutRates_seed{config['seed']}.csv",
                "def_mutRate": f"{output_dir}/defender_mutRates_seed{config['seed']}.csv",
            }
        else:
            files = {
                "atk_payoff": f"{output_dir}/attacker_payoffs_seed{config['seed']}.csv",
                "def_payoff": f"{output_dir}/defender_payoffs_seed{config['seed']}.csv",
                "atk_mutRate": f"{output_dir}/attacker_mutRates_seed{config['seed']}.csv",
                "def_mutRate": f"{output_dir}/defender_mutRates_seed{config['seed']}.csv",
                "atk_mutSdt": f"{output_dir}/attacker_sdts_seed{config['seed']}.csv",
                "def_mutSdt": f"{output_dir}/defender_sdts_seed{config['seed']}.csv",
            }


        for name, f_name in files.items():
            files[name] = open(f_name, "w")

        game.game_kernel(files, config, timing)

        for f in files.values():
            f.close()

    if timing: print(f"It took {time.time() - start} seconds")

if __name__ == "__main__":
    main_dir = os.path.join("experiment_yamls")
    main_output_dir = "paper_runs"

    for folder in os.listdir(main_dir):
        input_path = os.path.join(main_dir, folder)
        output_path = os.path.join(main_output_dir, folder)

        for filename in os.listdir(input_path):
            if "yaml" not in filename:
                continue

            full_input_path = os.path.join(input_path, filename)
            filename_no_extension = filename.split(".")[0]

            with open(full_input_path, "r") as f:
                data = yaml.safe_load(f)
            
            specific_config = data.get("config", {"": {}})
            for output_name, special_config in specific_config.items():
                full_output_path = os.path.join(output_path, filename_no_extension, output_name)
                os.makedirs(full_output_path, exist_ok=True)

                game_name = data["game"]
                config = data.get("shared_config", {}) | special_config

                try:
                    run_game(game_name, full_output_path, config)
                except Exception as e:
                    print(f"Raised the following ERROR: {e} \n\n With the config: \n{config}\n\n\n")
