library(ggplot2)
library(stringr)

res_BC <- read.csv2(file="results_BC.csv", sep = ",", dec = ".")
res_nogoodcuts <- read.csv2(file="results_nogoodcuts.csv", sep = ",", dec = ".")
res_MibS <- read.csv2(file="results_MibS.csv", sep = ",", dec = ".")

time_limit = 3600
res_BC[res_BC$SOLVE_STATUS == "TIME_LIMIT_REACHED","TIME"] <- 2*time_limit
res_nogoodcuts[res_nogoodcuts$SOLVE_STATUS == "TIME_LIMIT_REACHED","TIME"] <- 2*time_limit
res_MibS[res_MibS$SOLVE_STATUS == "TIME_LIMIT_REACHED","TIME"] <- 2*time_limit

print("proportion of nodes solved for BC, no-good cuts and MibS: ")
n_solved_BC = nrow(res_BC[res_BC$SOLVE_STATUS == "SOLUTION_FOUND",]) + nrow(res_BC[res_BC$SOLVE_STATUS == "NO_SOLUTION_FOUND",])
print(n_solved_BC / nrow(res_BC))
n_solved_nogoodcuts = nrow(res_nogoodcuts[res_nogoodcuts$SOLVE_STATUS == "SOLUTION_FOUND",]) + nrow(res_nogoodcuts[res_nogoodcuts$SOLVE_STATUS == "NO_SOLUTION_FOUND",])
print(n_solved_nogoodcuts / nrow(res_nogoodcuts))
n_solved_MibS = nrow(res_MibS[res_MibS$SOLVE_STATUS == "SOLUTION_FOUND",]) + nrow(res_MibS[res_MibS$SOLVE_STATUS == "NO_SOLUTION_FOUND",])
print(n_solved_MibS / nrow(res_MibS))

# average number of nodes (how do I count errors?)
print("average number of nodes including instances stopping because of time for BC, no-good cuts and MibS: ")
mean_nodes_BC = mean(res_BC$NODE_EXPLORED)
print(mean_nodes_BC)
mean_nodes_nogoodcuts = mean(res_nogoodcuts$NODE_EXPLORED)
print(mean_nodes_nogoodcuts)
mean_nodes_MibS = mean(res_MibS$NODE_EXPLORED)
print(mean_nodes_MibS)

# number of errors
print("number of instances that errored for BC, no-good cuts and MibS: ")
n_errors_BC = nrow(res_BC[substring(res_BC$SOLVE_STATUS,1,5) == "ERROR",])
print(n_errors_BC)
n_errors_nogoodcuts = nrow(res_nogoodcuts[substring(res_BC$SOLVE_STATUS,1,5) == "ERROR",])
print(n_errors_nogoodcuts)
n_errors_MibS = nrow(res_MibS[substring(res_MibS$SOLVE_STATUS,1,5) == "ERROR",])
print(n_errors_MibS)


# compute average and median of computation time among instances solved by all methods
compute_common_times <- function(m1, m2, m3,
                                 instance_col = "INSTANCE") {
  
  solved_statuses <- c("NO_SOLUTION_FOUND", "SOLUTION_FOUND")
  
  m1 <- m1[m1$SOLVE_STATUS %in% solved_statuses, ]
  m2 <- m2[m2$SOLVE_STATUS %in% solved_statuses, ]
  m3 <- m3[m3$SOLVE_STATUS %in% solved_statuses, ]
  
  common_instances <- Reduce(
    intersect,
    list(
      m1[[instance_col]],
      m2[[instance_col]],
      m3[[instance_col]]
    )
  )
  
  m1 <- m1[m1[[instance_col]] %in% common_instances, ]
  m2 <- m2[m2[[instance_col]] %in% common_instances, ]
  m3 <- m3[m3[[instance_col]] %in% common_instances, ]
  
  result <- data.frame(
    Method = c("BC", "no-good cuts", "MibS"),
    N = c(nrow(m1), nrow(m2), nrow(m3)),
    Average = c(mean(m1$TIME), mean(m2$TIME), mean(m3$TIME)),
    Median = c(median(m1$TIME), median(m2$TIME), median(m3$TIME))
  )
  
  return(result)
}


result <- compute_common_times(res_BC, res_nogoodcuts, res_MibS)
print(result)