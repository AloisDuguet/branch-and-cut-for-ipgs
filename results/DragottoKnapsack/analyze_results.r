library(ggplot2)
library(stringr)

res <- read.csv2(file="results.csv", sep = ",", dec = ".")

time_limit = 1800
res[res$SOLVE_STATUS == "TIME_LIMIT_REACHED","TIME"] <- 2*time_limit
nrow(res[res$SOLVE_STATUS == "TIME_LIMIT_REACHED",])


print("instances of 2 players and 25 items solved with basicAlgorithm (out of 9)")
print(nrow(res[res$ALGORITHM_VARIANT == "basicAlgorithm" & substring(res$FILENAME,42,43) == "25" & substring(res$FILENAME,40,40) == "2" & res$TIME < time_limit,]))
print("instances of 2 players and 50 items solved with basicAlgorithm (out of 9)")
print(nrow(res[res$ALGORITHM_VARIANT == "basicAlgorithm" & substring(res$FILENAME,42,43) == "50" & substring(res$FILENAME,40,40) == "2" & res$TIME < time_limit,]))
print("instances of 2 players and 50 items solved with cuttingBeforeBranching (out of 9)")
print(nrow(res[res$ALGORITHM_VARIANT == "cuttingBeforeBranching" & substring(res$FILENAME,42,43) == "50" & substring(res$FILENAME,40,40) == "2" & res$TIME < time_limit,]))
print("instances of 2 players and 75 items solved with basicAlgorithm (out of 9)")
print(nrow(res[res$ALGORITHM_VARIANT == "basicAlgorithm" & substring(res$FILENAME,42,43) == "75" & substring(res$FILENAME,40,40) == "2" & res$TIME < time_limit,]))


print("proportion of instances solved in the time limit:")
count_solved <- nrow(res[res$SOLVE_STATUS == "SOLUTION_FOUND",]) + nrow(res[res$SOLVE_STATUS == "NO_SOLUTION_FOUND",])
print(count_solved/nrow(res))
