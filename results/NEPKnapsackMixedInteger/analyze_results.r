library(ggplot2)
library(stringr)

res <- read.csv2(file="results.csv", sep = ",", dec = ".")

time_limit = 3600
res[res$SOLVE_STATUS == "TIME_LIMIT_REACHED","TIME"] <- 2*time_limit
nrow(res[res$SOLVE_STATUS == "TIME_LIMIT_REACHED",])


print("proportion of instances solved in less than 1 second:")
count_solved_1sec <- nrow(res[res$SOLVE_STATUS == "SOLUTION_FOUND" & res$TIME <= 1,]) + nrow(res[res$SOLVE_STATUS == "NO_SOLUTION_FOUND" & res$TIME <= 1,])
print(count_solved_1sec/nrow(res))
print("proportion of instances solved in the time limit:")
count_solved <- nrow(res[res$SOLVE_STATUS == "SOLUTION_FOUND",]) + nrow(res[res$SOLVE_STATUS == "NO_SOLUTION_FOUND",])
print(count_solved/nrow(res))
print("number of instances solved with no NE proved:")
print(nrow(res[res$SOLVE_STATUS == "NO_SOLUTION_FOUND",]))


print("table with proportion of solved wrt items, number of players and game type")
game_types = c("NEP-mixedInteger")
num_players = c(2,3,4)
num_items = c("5-","10","15","20","30","40","50","60","70","80","all")
num_items_real = c("5","10","15","20","30","40","50","60","70","80","all")
game_type = game_types[1]
print(game_type)
solved <- matrix(1:33, nrow = 3, ncol = 11)
for (p in 1:3) {
  num_player <- num_players[p]
  for (j in 1:10) {
    num_item <- num_items[j]
    sub <- res[res$GAME_TYPE == game_type & substring(res$FILENAME,45,46) == num_item & substring(res$FILENAME,43,43) == num_player,]
    solved[p,j] <- sum(sub[,"SOLVE_STATUS"] == "SOLUTION_FOUND") + sum(sub[,"SOLVE_STATUS"] == "NO_SOLUTION_FOUND")
  }
  sub <- res[res$GAME_TYPE == game_type & substring(res$FILENAME,43,43) == num_player,]
  solved[p,j+1] <- round(100*(sum(sub[,"SOLVE_STATUS"] == "SOLUTION_FOUND") + sum(sub[,"SOLVE_STATUS"] == "NO_SOLUTION_FOUND")) / nrow(sub))
}
tab_solved <- as.table(solved)
dimnames(tab_solved) <- list(players = num_players, items = num_items_real)
print(tab_solved)
write.table(tab_solved, paste("table_solved_",game_type))

print("maximum number of nodes in one instance:")
print(max(res$NODE_EXPLORED))
print("minimum number of nodes in one unsolved instance:")
print(min(res[res$SOLVE_STATUS == "TIME_LIMIT_REACHED","NODE_EXPLORED"]))
