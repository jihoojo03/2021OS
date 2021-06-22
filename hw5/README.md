# tfind : multithreaded text search program


<h2> Introduction </h2>
<div>
You are asked to design and construct tfind, a multithreaded program for finding all lines of given keywords from all text files under a certain directory. To utilize parallelism of a multi-core processor, tfind creates and operates one or multiple worker threads to conduct text searching on the files in multiple directories concurrently.</div>
 <br>
 
<h2> Make Excution File </h2>

     $ gcc -pthread tfind.c -o tfind

<h2> About Command Line </h2>

     $ ./tfind -t <num> <dir> [<keyword>]+ 

<h4> The options for pfind </h4>

  >**-t** **/Num/** tfind receives a positive integer (i e , <num>) as the number of the worker threads (no more than 16) <br>
  >**/dir/** You can put any absolute or relative directory name! <br>
<h4> Other Options </h4>

  > tfind should search in, and a non-empty list of keywords (i e , one or more <keyword>’s up to 8) that t find should search for within a single line
 
<h4> Examples </h4>

  > ./tfind -t 3 ./dir1 asdf <br>
  > ./tfind -t 2 /usr/local/llvm/lib comm as b w <br>
  > ./tfind -t 15 /usr/local/llvm/lib comm as b w
  
 <h4> Result Examples </h4>
 ![image](https://user-images.githubusercontent.com/59548055/122989193-7e972580-d3dd-11eb-896b-7fb2c93d3437.png)

<br>
<h2> Demo Link </h2>
https://github.com/jihoojo03/2021OS/tree/main/hw5

<h2> Video Link </h2>
https://youtu.be/VVF1D-FXU40
