library(ggplot2)
library(stringr)

subTimes <- read.csv2(file="subTimes.csv", sep = ",", dec = ".")

# compute mean of proportion of time spent in cutDerivation/BRComputation/NodeSolve wrt total time
cutProportions <- subTimes[,"totalCutDerivationTime"]/subTimes[,"X.computation.time."]
BRProportions <- subTimes[,"totalBest"]/subTimes[,"X.computation.time."]
NodeProportions <- subTimes[,"totalNode"]/subTimes[,"X.computation.time."]
print(paste("mean proportion of time spent in derivation of cuts: ", sum(cutProportions)/length(cutProportions)))
print(paste("mean proportion of time spent in computation of BR: ", sum(BRProportions)/length(BRProportions)))
print(paste("mean proportion of time spent in computation of node: ", sum(NodeProportions)/length(NodeProportions)))

# plot proportion of time spent per instance
proportions <- data.frame(y = c(cutProportions, BRProportions, NodeProportions), number = 1:length(cutProportions), Instances = factor(c(rep(0,length(cutProportions)), rep(1, length(BRProportions)), rep(2, length(NodeProportions))), 0:2, labels = list("cuts","BR","NodeSolve")))
ggplot(proportions, aes(x=number,col=Instances,linetype=Instances,y=y)) +
  coord_cartesian(xlim = c(1,length(cutProportions)), ylim = c(0,1), expand = TRUE) +
  scale_y_continuous() +
  scale_color_manual(values = c("cuts" = "red", "BR" = "blue", "NodeSolve" = "green")) +
  geom_point()
ggsave("Proportion of time spent in cuts-BR-nodeSolve.pdf", width = 4, height = 3.4)

# compute mean of proportion of time spent in different steps of cut derivation (specific to intersection cuts) wrt cut derivation time
rayBuilding <- subTimes[subTimes$totalCutDerivationTime > 0.0,"rayBuildingTime"]/subTimes[subTimes$totalCutDerivationTime > 0.0,"totalCutDerivationTime"]
NEFreeSet <- subTimes[subTimes$totalCutDerivationTime > 0.0,"NEFreeSet"]/subTimes[subTimes$totalCutDerivationTime > 0.0,"totalCutDerivationTime"]
FinalStepIC <- subTimes[subTimes$totalCutDerivationTime > 0.0,"FinalStepIC"]/subTimes[subTimes$totalCutDerivationTime > 0.0,"totalCutDerivationTime"]
print(paste("number of instances with cuts derived out of ", length(cutProportions), " is ", length(rayBuilding)))
print(paste("mean proportion of time spent in ray building among cut derivation time: ", sum(rayBuilding)/length(rayBuilding)))
print(paste("mean proportion of time spent in NE-free set building among cut derivation time: ", sum(NEFreeSet)/length(rayBuilding)))
print(paste("mean proportion of time spent in computing alpha and cut formula among cut derivation time: ", sum(FinalStepIC)/length(rayBuilding)))
