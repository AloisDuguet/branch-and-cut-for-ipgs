library(ggplot2)
library(stringr)

res <- read.csv2(file="results.csv", sep = ",", dec = ".")

time_limit = 3600
res[res$SOLVE_STATUS == "TIME_LIMIT_REACHED","TIME"] <- 2*time_limit
res[grep("ERROR",res$SOLVE_STATUS),"TIME"] <- 2*time_limit
print("number of instances stopped because of an error:")
nrow(res[grep("ERROR",res$SOLVE_STATUS),])


sub1 <- res[res$SOLVE_STATUS == "SOLUTION_FOUND" | res$SOLVE_STATUS == "NO_SOLUTION_FOUND","NODE_EXPLORED"]
sub2 <- res[res$SOLVE_STATUS == "TIME_LIMIT_REACHED","NODE_EXPLORED"]
max_nodes <- max(res$NODE_EXPLORED)
print("maximum number of nodes explored in one instance:")
print(max_nodes)
data_ECDF_general <- data.frame(x = c(sub1,sub2), Instances = factor(c(rep(0,length(sub1)),rep(1,length(sub2))), 0:1, labels = list("unsolved","solved")))
ggplot(data_ECDF_general, aes(x=x,col=Instances,linetype=Instances)) +
  scale_linetype_manual(breaks=c("unsolved","solved"),values=c(1,2)) +
  scale_x_continuous(trans = 'log10') + 
  coord_cartesian(xlim = c(1,max_nodes), expand = TRUE) +
  theme_gray(base_size = 14) + stat_ecdf(linewidth=1.2) + theme(legend.text = element_text(size=15), legend.title = element_text(size=15), axis.text=element_text(size=15), legend.position="none", axis.title.x = element_blank(), axis.title.y = element_blank())
ggsave("ECDF_NODE_all_instances.pdf", width = 4, height = 3.4)

sub1 <- res[res$SOLVE_STATUS == "SOLUTION_FOUND" | res$SOLVE_STATUS == "NO_SOLUTION_FOUND","CUTS_ADDED"] + 0.01
sub2 <- res[res$SOLVE_STATUS == "TIME_LIMIT_REACHED","CUTS_ADDED"] + 0.01
max_cuts <- max(res$CUTS_ADDED)
print("maximum number of cuts derived in one instance:")
print(max_cuts)
data_ECDF_general <- data.frame(x = c(sub1,sub2), Instances = factor(c(rep(0,length(sub1)),rep(1,length(sub2))), 0:1, labels = list("unsolved","solved")))
ggplot(data_ECDF_general, aes(x=x,col=Instances,linetype=Instances)) +
  scale_linetype_manual(breaks=c("unsolved","solved"),values=c(1,2)) +
  scale_x_continuous(trans = 'log10') + 
  coord_cartesian(xlim = c(1,max_cuts), expand = TRUE) +
  stat_ecdf(linewidth=1.2) + theme(legend.text = element_text(size=15), legend.title = element_text(size=15), axis.text=element_text(size=15), legend.position="none", axis.title.x = element_blank(), axis.title.y = element_blank())
ggsave("ECDF_CUT_all_instances.pdf", width = 4, height = 3.4)


print("table with proportion of solved wrt nodes, number of players and game type")
game_types = c("implementationGame")
num_players = c("I_2","I_4","I_10")
num_nodes = c("[0-9]_10_[ms]","[0-9]_15_[ms]","[0-9]_20_[ms]")
num_nodes_real = c(10,15,20)
game_type = game_types[1]
for (i in 1:1) {
  game_type <- game_types[i]
  print(game_type)
  solved <- matrix(1:9, nrow = 3, ncol = 3)
  for (p in 1:3) {
    num_player <- num_players[p]
    for (j in 1:3) {
      num_node <- num_nodes[j]
      sub <- res
      sub <- sub[grep(num_node,sub$FILENAME),]
      sub <- sub[grep(num_player,sub$FILENAME),]
      solved[p,j] <- round(100*sum(sub[sub$GAME_TYPE == game_type,"SOLVE_STATUS"] != "TIME_LIMIT_REACHED") / nrow(sub), 1)
    }
  }
  tab_solved <- as.table(solved)
  dimnames(tab_solved) <- list(players = num_players, items = num_nodes_real)
  print(tab_solved)
  write.table(tab_solved, paste("table_solved_",game_type))
}

print("proportion of instances solved in less than 1 second:")
count_solved_1sec <- nrow(res[res$SOLVE_STATUS == "SOLUTION_FOUND" & res$TIME <= 1,]) + nrow(res[res$SOLVE_STATUS == "NO_SOLUTION_FOUND" & res$TIME <= 1,])
print(count_solved_1sec/nrow(res))
print("proportion of instances solved in the time limit:")
count_solved <- nrow(res[res$SOLVE_STATUS == "SOLUTION_FOUND",]) + nrow(res[res$SOLVE_STATUS == "NO_SOLUTION_FOUND",])
print(count_solved/nrow(res))


print("proportion of instances that found an NE:")
print(nrow(res[res$SOLVE_STATUS == "SOLUTION_FOUND",])/nrow(res))

print("proportion of instances that proved no NE exists:")
print(nrow(res[res$SOLVE_STATUS == "NO_SOLUTION_FOUND",])/nrow(res))

print("minimum number of cuts for instances that proved no NE exists:")
print(min(res[res$SOLVE_STATUS == "NO_SOLUTION_FOUND","CUTS_ADDED"]))

print("proportion of solved instances that are solved in the root node:")
print(nrow(res[res$SOLVE_STATUS != "TIME_LIMIT_REACHED" & res$NODE_EXPLORED == 1,])/nrow(res[res$SOLVE_STATUS != "TIME_LIMIT_REACHED",]))

print("proportion of solved instances that have no cuts derived:")
print(nrow(res[res$SOLVE_STATUS != "TIME_LIMIT_REACHED" & res$CUTS_ADDED == 0,])/nrow(res[res$SOLVE_STATUS != "TIME_LIMIT_REACHED",]))

print("proportion of instances not solved that have no cuts derived:")
print(nrow(res[res$SOLVE_STATUS == "TIME_LIMIT_REACHED" & res$CUTS_ADDED == 0,])/nrow(res[res$SOLVE_STATUS == "TIME_LIMIT_REACHED",]))
