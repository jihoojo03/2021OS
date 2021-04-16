# pfind: parallel text search program using multiprocessing


<h2> Introduction </h2>
<div>
In this homework, you are asked to design and construct a text searching program 'pfind' which find all lines of given keywords from all text files under a certain directory. For time performance, 'pfind' is designed to run one or multiple children processes as workers which conduct text searching on multiple directories concurrently. The main process communicates with workers via named pipes (i.e., fifo) to give jobs and then receive the results.
 </div>
 <br>

<h2> About Command Line </h2>

     $ ./pfind [<option>]* <dir> [<keyword>] 

<h4> The options for pfind </h4>

  >**-p** **/N/** defines the number of the worker processes (i.e., child processes) as N which be greater than 0 and less than 10. The default value is 2. <br>
  >**-a**   print the absolute path of a file. The default behavior is to print a relative path.

<h4> Other Options </h4>

> pfind receives the path to a directory for which searching will be conducted (i.e., /dir/) and a keyword (i.e., one /keyword/). For given directory path and keywords, pfind  must examine all text files under the given directory to find every line that contains a given keyword at the same time. Once pfind detects a line that meets the condition, it  prints out the path, the filename, the line number and the content of the line to the standard output.

<br>

<h2> Manager-workers architectures </h2>
<div>
For parallelization, pfind divides a searching job into multiple tasks each of which is to explore a single directory. A worker takes on one task at a time, thus, multiple tasks can be processed concurrently on multiple workers. The manager and the workers communicate via two pipes: left one for passing tasks from the manager to the workers, and right one for receiving task results from the workers to the manager.
</div>

<br>

<h2> Checking Test Results </h2>
   
 The main process (i.e., first process) of pfind roles as the manager which takes the following activities:
  1. receive commands from the user, and
  2. spawn child processes as workers, and
  3. assign tasks to the workers, and
  4. receive the task output from the workers, and
  5. print the search summary, and
  6. terminate the workers before the program termination.


<br>

<h2> Demo Link </h2>
https://github.com/jihoojo03/2021OS/tree/main/hw3
