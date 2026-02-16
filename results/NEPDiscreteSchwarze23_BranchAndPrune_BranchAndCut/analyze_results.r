library(ggplot2)
library(stringr)
library(xtable)

resBC <- read.csv2(file="resultsBC.csv", sep = ",", dec = ".")
resBP <- read.csv2(file="resultsBP.csv", sep = ",", dec = ".")

resBP$t1 <- as.numeric(resBP$t1)
resBP$t3 <- as.numeric(resBP$t3)
resBC$TIME <- as.numeric(resBC$TIME)

BPinstances = resBP$instance
for (i in 1:length(BPinstances))
  BPinstances[i] = sprintf("$%s$", BPinstances[i])

BPstatus = resBP$t1
for (i in 1:length(BPstatus)) {
  if (as.numeric(resBP$t1[i]) == -1) {
    BPstatus[i] = "error"
  } else if (as.numeric(resBP$t1[i]) > 0) {
    BPstatus[i] = "NE found"
  } else if (as.numeric(resBP$t3[i]) > 3600) {
    BPstatus[i] = "$t_{max}$"
  } else {
    BPstatus[i] = "no NE"
  }
}

BPtime = resBP$t3
for (i in 1:length(BPtime)) {
  if (BPstatus[i] == "NE found") {
    BPtime[i] = round(resBP$t1[i], digits=1)
  }
  else if (is.na(resBP$t3[i])) {
    BPtime[i] = '-'
  }
  else if (resBP$t3[i] >= 3600) {
    BPtime[i] = "-"
  }
  else
    BPtime[i] = round(resBP$t3[i], digits=1)
}

BCstatus = resBC$SOLVE_STATUS
for (i in 1:length(BCstatus)) {
  if (BCstatus[i] == "NO_SOLUTION_FOUND")
    BCstatus[i] = "no NE"
  else if (BCstatus[i] == "SOLUTION_FOUND")
    BCstatus[i] = "NE found"
  else
    BCstatus[i] = "$t_{max}$"
}

BCtime = resBC$TIME
for (i in 1:length(BCtime)) {
  if (as.numeric(BCtime[i]) >= 3599.9)
    BCtime[i] = "-"
  else
    BCtime[i] = round(as.numeric(BCtime[i]), digits=1)
}

table <- data.frame(instance = BPinstances, Bstatus = BPstatus, BPtime = BPtime, BCstatus = BCstatus, BCtime = BCtime, nodes = resBC$NODE_EXPLORED, cuts = resBC$CUTS_ADDED)
rownames(table) <- NULL
print(xtable(table, type='latex'))

countInstances <- nrow(table)
print("number and fraction of instances for which an NE was found or no NE was proved for BP:")
countSolvedBP <- countInstances - nrow(table[table$BPstatus == "error",]) - nrow(table[table$BPstatus == "$t_{max}$",])
fracSolvedBP <- countSolvedBP / countInstances
print(paste(countSolvedBP," ",fracSolvedBP))
print("number and fraction of instances for which an NE was found or no NE was proved for BC:")
countSolvedBC <- countInstances - nrow(table[table$BCstatus == "error",]) - nrow(table[table$BCstatus == "$t_{max}$",])
fracSolvedBC <- countSolvedBC / countInstances
print(paste(countSolvedBC," ",fracSolvedBC))

# compute ratio of time when the two found a solution
logratios = list()
sumlogratios = 0
for (i in 1:length(BCtime)) {
  if (BPstatus[i] == BCstatus[i] & !is.na(BPstatus[i]) & BPstatus[i] != "$t_{max}$") {
    ratio = as.numeric(BPtime[i]) / as.numeric(BCtime[i])
    logratios = append(logratios,log(ratio))
    sumlogratios = sumlogratios + log(ratio)
  }
}
meanGeometricRatio = exp(sumlogratios/length(logratios))

ratios = list()
sumratios = 0
for (i in 1:length(BCtime)) {
  if (BPstatus[i] == BCstatus[i] & !is.na(BPstatus[i]) & BPstatus[i] != "$t_{max}$") {
    ratio = as.numeric(BPtime[i]) / as.numeric(BCtime[i])
    ratios = append(ratios,ratio)
    sumratios = sumratios + ratio
  }
}
meanArithmeticRatio = sumratios/length(ratios)
