def read_scenarios(filePath):
    scenarios     = []
    curr_scenario = []

    with open(filePath, 'r') as input_file:
        for line in input_file:
            line = line.rstrip()
            if line.startswith('#'):
                if curr_scenario:
                    scenarios.append(curr_scenario)
                    curr_scenario = []
            elif line:
                curr_scenario.append(line.split(", "))
        if curr_scenario:
            scenarios.append(curr_scenario)
    return scenarios
