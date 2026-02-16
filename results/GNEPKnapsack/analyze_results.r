library(ggplot2)
library(stringr)

res <- read.csv2(file="results.csv", sep = ",", dec = ".")

time_limit = 3600
res[res$SOLVE_STATUS == "TIME_LIMIT_REACHED","TIME"] <- 2*time_limit
nrow(res[res$SOLVE_STATUS == "TIME_LIMIT_REACHED",])

# remove results with high capacity because they are 'too easy'
res <- res[grep("-[25]-[uws]", res$FILENAME),]

# figure
sub1 <- res[res$SOLVE_STATUS == "SOLUTION_FOUND" | res$SOLVE_STATUS == "NO_SOLUTION_FOUND","CUTS_ADDED"] + 0.01
sub2 <- res[res$SOLVE_STATUS == "TIME_LIMIT_REACHED","CUTS_ADDED"] + 0.01
max_cuts <- max(res$CUTS_ADDED)
print("maximum number of cuts in one instance:")
print(max_cuts)
data_ECDF_general <- data.frame(x = c(sub1,sub2), Instances = factor(c(rep(0,length(sub1)),rep(1,length(sub2))), 0:1, labels = list("unsolved","solved")))
ggplot(data_ECDF_general, aes(x=x,col=Instances,linetype=Instances)) +
  scale_linetype_manual(breaks=c("unsolved","solved"),values=c(1,2)) +
  scale_x_continuous(trans = 'log10') + 
  coord_cartesian(xlim = c(1,max_cuts), expand = TRUE) +
  stat_ecdf(linewidth=1.2) + theme(legend.text = element_text(size=15), legend.title = element_text(size=15), axis.text=element_text(size=15), legend.position="none", axis.title.x = element_blank(), axis.title.y = element_blank())
ggsave("ECDF_CUT_all_instances.pdf", width = 4, height = 3.4)



# statistics
print("proportion of instances solved in less than 1 second:")
count_solved <- nrow(res[res$SOLVE_STATUS == "SOLUTION_FOUND" & res$TIME <= 1,]) + nrow(res[res$SOLVE_STATUS == "NO_SOLUTION_FOUND" & res$TIME <= 1,])
print(count_solved/nrow(res))
print("proportion of instances solved in the time limit:")
count_solved <- nrow(res[res$SOLVE_STATUS == "SOLUTION_FOUND",]) + nrow(res[res$SOLVE_STATUS == "NO_SOLUTION_FOUND",])
print(count_solved/nrow(res))
print("number of instances with no NE proved:")
print(nrow(res[res$SOLVE_STATUS == "NO_SOLUTION_FOUND",]))


print("table with proportion of solved wrt items, number of players and game type")
game_types = c("GNEP-fullInteger")
num_players = c(2,3,4)
num_items = c("5-","10","15","20","30","40","50","all")
num_items_real = c("5","10","15","20","30","40","50","all")
game_type = game_types[1]
for (i in 1:1) {
  game_type <- game_types[i]
  print(game_type)
  solved <- matrix(1:24, nrow = 3, ncol = 8)
  for (p in 1:3) {
    num_player <- num_players[p]
    for (j in 1:7) {
      num_item <- num_items[j]
      solved[p,j] <- sum(res[res$GAME_TYPE == game_type & substring(res$FILENAME,46,47) == num_item & substring(res$FILENAME,44,44) == num_player,"SOLVE_STATUS"] != "TIME_LIMIT_REACHED")
    }
    sub <- res[res$GAME_TYPE == game_type & substring(res$FILENAME,44,44) == num_player,]
    solved[p,j+1] <- round(100*(sum(sub[,"SOLVE_STATUS"] == "SOLUTION_FOUND") + sum(sub[,"SOLVE_STATUS"] == "NO_SOLUTION_FOUND")) / nrow(sub))
  }
  tab_solved <- as.table(solved)
  dimnames(tab_solved) <- list(players = num_players, items = num_items_real)
  print(tab_solved)
  write.table(tab_solved, paste("table_solved_",game_type))
}
