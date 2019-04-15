# apue-2017302580093-hw2

# NOTE

1. Remove -Wall gcc flag, since it complains about 'lstat' function.

2. -F: In Ubuntu, there is no 'whiteout'.

3. There are surely some differences between my ls program and Ubuntu's. Some of them exist because the homework specifications are based on BSD system, not Linux. Others exist because there are no specific restrictions related to them in the homework specifications. 

(For example, in the homework specifications, the author didn't mention whether or not the output of the final ls program should have newlines before and after it, so I chose to print newlines before and after the output because it's more consistent with the general ls output format. The ls program under Ubuntu system doesn't do that.) 
