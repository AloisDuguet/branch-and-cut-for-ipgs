library(ggplot2)
library(stringr)

res <- read.csv2(file="results.csv", sep = ",", dec = ".")

time_limit = 3600
res[res$SOLVE_STATUS == "TIME_LIMIT_REACHED","TIME"] <- 2*time_limit


sub1 <- res[res$SOLVE_STATUS == "SOLUTION_FOUND" | res$SOLVE_STATUS == "NO_SOLUTION_FOUND","NODE_EXPLORED"]
sub2 <- res[res$SOLVE_STATUS == "TIME_LIMIT_REACHED","NODE_EXPLORED"]
max_nodes <- max(res$NODE_EXPLORED)
data_ECDF_general <- data.frame(x = c(sub1,sub2), Instances = factor(c(rep(0,length(sub1)),rep(1,length(sub2))), 0:1, labels = list("unsolved","solved")))
ggplot(data_ECDF_general, aes(x=x,col=Instances,linetype=Instances)) +
  scale_linetype_manual(breaks=c("unsolved","solved"),values=c(1,2)) +
  scale_x_continuous(trans = 'log10') + 
  coord_cartesian(xlim = c(1,max_nodes), expand = TRUE) +
  stat_ecdf(linewidth=1.2) + theme(legend.text = element_text(size=15), legend.title = element_text(size=15), axis.text=element_text(size=15), legend.position="none", axis.title.x = element_blank(), axis.title.y = element_blank())
ggsave("ECDF_NODE_all_instances.pdf", width = 4, height = 3.4)

sub1 <- res[res$SOLVE_STATUS == "SOLUTION_FOUND" | res$SOLVE_STATUS == "NO_SOLUTION_FOUND","CUTS_ADDED"] + 0.01
sub2 <- res[res$SOLVE_STATUS == "TIME_LIMIT_REACHED","CUTS_ADDED"] + 0.01
max_cuts <- max(res$CUTS_ADDED)
data_ECDF_general <- data.frame(x = c(sub1,sub2), Instances = factor(c(rep(0,length(sub1)),rep(1,length(sub2))), 0:1, labels = list("unsolved","solved")))
ggplot(data_ECDF_general, aes(x=x,col=Instances,linetype=Instances)) +
  scale_linetype_manual(breaks=c("unsolved","solved"),values=c(1,2)) +
  scale_x_continuous(trans = 'log10') + 
  coord_cartesian(xlim = c(1,max_cuts), expand = TRUE) +
  stat_ecdf(linewidth=1.2) + theme(legend.text = element_text(size=15), legend.title = element_text(size=15), axis.text=element_text(size=15), legend.position="none", axis.title.x = element_blank(), axis.title.y = element_blank())
ggsave("ECDF_CUT_all_instances.pdf", width = 4, height = 3.4)

res_items <- res
sub1 <- res_items[grep("[234]-5-[258]",res_items$FILENAME),"TIME"]
sub2 <- res_items[grep("10-",res_items$FILENAME),"TIME"]
sub3 <- res_items[grep("15",res_items$FILENAME),"TIME"]
sub4 <- res_items[grep("20",res_items$FILENAME),"TIME"]
sub5 <- res_items[grep("30",res_items$FILENAME),"TIME"]
sub6 <- res_items[grep("40",res_items$FILENAME),"TIME"]
sub7 <- res_items[grep("50",res_items$FILENAME),"TIME"]
sub8 <- res_items[grep("60",res_items$FILENAME),"TIME"]
sub9 <- res_items[grep("70",res_items$FILENAME),"TIME"]
sub10 <- res_items[grep("80",res_items$FILENAME),"TIME"]
sub2set_items <- data.frame(x = c(sub1,sub2,sub3,sub4,sub5,sub6,sub7,sub8,sub9,sub10), items = gl(10,length(sub1), labels = list(5,10,15,20,30,40,50,60,70,80)))
ggplot(sub2set_items, aes(x=x,col=items)) +
  stat_ecdf(linewidth=1.2) + theme(legend.text = element_text(size=15), legend.title = element_text(size=15), axis.text=element_text(size=15), axis.title.x = element_blank(), axis.title.y = element_blank()) + 
  scale_x_continuous(trans = 'log10') +
  coord_cartesian(xlim = c(0.01,time_limit*0.9), expand = TRUE) +
  labs(x = "Computation time", y = "ECDF")
ggsave("ECDF_items.pdf", width = 4, height = 3.4)


print("proportion of instances solved in less than 1 second:")
count_solved_1sec <- nrow(res[res$SOLVE_STATUS == "SOLUTION_FOUND" & res$TIME <= 1,]) + nrow(res[res$SOLVE_STATUS == "NO_SOLUTION_FOUND" & res$TIME <= 1,])
print(count_solved_1sec/nrow(res))
print("proportion of instances solved in the time limit:")
count_solved <- nrow(res[res$SOLVE_STATUS == "SOLUTION_FOUND",]) + nrow(res[res$SOLVE_STATUS == "NO_SOLUTION_FOUND",])
print(count_solved/nrow(res))
print("number of instances solved with no NE proved:")
print(nrow(res[res$SOLVE_STATUS == "NO_SOLUTION_FOUND",]))

print("proportion of instances with 5 items solved in less than 1 second:")
res_5items <- res[grep("[234]-5-[258]",res$FILENAME),]
count_solved <- nrow(res_5items[res_5items$TIME < 1,])
print(count_solved/nrow(res_5items))

print("minimum number of cuts of an unsolved instance: ")
print(min(res[res$SOLVE_STATUS == "TIME_LIMIT_REACHED","CUTS_ADDED"]))


print("table with proportion of solved wrt items, number of players and game type")
game_types = c("NEP-fullInteger")
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
