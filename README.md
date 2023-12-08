NOTE!
must use "bash ./runit.sh" to run properly

---self-assesment---
1. 8 points. Read 100 bytes from start of file                                                  8/8
2. 3 points. Read 200 bytes, from FBN 1, at cursor = 30                                         3/3
3. 3 points. Read 1,000 bytes from start of DBN 20 (spanning read)                              3/3
4. 6 points. Write 77 bytes, starting at 10 bytes into FBN 7                                    6/6
5. 5 points. Write 900 bytes, starting at 50 bytes into DBN 10                                  5/5
6. Bonus 10 points. Write 700 bytes, starting at FBN 49. (This write extends the file)          10/0

Total                                                                                           35/25
All test pass

---assumptions---
- fd given is valid
- most errors are checked for within my functions or the functions already provided
