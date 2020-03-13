### Watdfs Distributed System

---

### Overview

This is the implementation of a distributed file system that described in the paper: https://cs.uwaterloo.ca/~mtabebe/publications/abebeWatDFS2019SIGCSE.pdf

Below is a diagram that traces the steps (indicated by numbered arrows) that a file system operation (such as reading a file) invoked by an application (e.g., the cat command) will take to reach WatDFS, as well as the actions that WatDFS should take. The dashed lines indicate separate processes, the thick gray lines separate machine boundaries, and the thin gray lines separate user space from the kernel.  

I have implemented both the client and server components with features including mutual exclusion, file freshness check and atomic read & write ...

<img src="/Users/gabriel/Desktop/Screen Shot 2020-03-13 at 7.37.57 PM.png" alt="Screen Shot 2020-03-13 at 7.37.57 PM" style="zoom:30%;" />

####  1. Ideas of Design and Functionalities



##### 1.1 client and server global variables

I have implemented two global structure variable to keep track of the state and metadata of each file on client side. 

```c
struct fileMetadata {
  int client_mode;
  int server_mode;
  time_t tc;
};
```

`fileMetadata` tracks the `fh` and file access flags information. It also keeps track of the last access time of current file. Any open or modification to a file, might change the metadata.

```c
struct file_state {
  time_t cacheInterval;
  char *cachePath;
  std::map<std::string, struct fileMetadata > openFiles;
};
```

It is important to know the syncrhonization status of a file on both client and server side and we better need a `openFiles` map data structure to record files opened by the client side. `CachePath` is simply the path of a file and `cacheInterval` is mainly used to check the freshness. Overall, we say this structure is the global state of all target files (`userdata`).



On server side, i decide to create a global structure to keep track of 3 states of a file (read only, write and read, closed). And also a map to track all fils opened from client side. Those are intialized during server call to `watdfs_open`

```c
struct file_critical_section{
    bool read; // true for r and false for w
    rw_lock_t * lock ;
};

std::map<std::string, struct file_critical_section> fileMutex;
```



##### 1.2 Download to server / Upload from server



> int download_to_client(struct file_state *userdata, const char *full_path, const char *path);

1. initalize `stat buffer` on server side and client will trigger a `open rpc call` to read file information from server and copy to the local side if the file on server exists.
2. Call `open` system call to open file from local side. this might results in creation (`mknod` system call) of new file if it does not exist.
3. After local file is opened, truncate the local file based on the required size limit from server. and then trigger `read rpc call` to read from server so that we can then write all data and stored in the local file using `pwrite` system call function.
4. Update file metadata accordingly.



> int push_to_server(struct file_state *userdata, const char *full_path, const char *path);

1. same as above.
2. `open rpc call` is triggered to open the file on server. If not exist, we will create a new one and then open on the server.
3. We will open the target open file that already exists through system call `open` and then read all data through `pread`.
4. Trigger `truncate rpc call` to truncate the server file based on size information of local file. So it gets ready for receiving the data.
5. Now, it is ready to write the data from local to server through `write rpc call.`
6. Update metadata accordingly.

7. Try to get the attribute of this file on server and test to see if above steps are functioned correctly.





##### 1.3 Atomic Transfer of files and Mutual Exclusions

>Atomic Transfer:

In addition to above `download` and `upload` explanations, the client side is forced to try to achieve and hold the `lock_to_read` before and during the file downloading to client and achieve and hold the `lock_to_write` before and during the file uploading to server process. Locks are released once the download and upload period is each done. 

With this implementation feature, file is either transfer completed or not at all as others wont be able to interrupt at the middle of transferring.

> Mutual Exclusion:

Recall that client side has a map structure to keep track of all opened files by client. When a `open` call is trigger to a file, we will first examine if it is opened already by others. This can prevent the sitation where mutiple files are trying to access the same file at the same time. This ensures also mutual exclusions.

Recall that the server also have a map to track all files opened. To ensure the mutual exclusions of server write, if the file is already opened for writing, any futher open call will result in an error code returned.



##### 1.4 Cache Invalidation - Time-Out Based

The key idea to check if the operatio on file satisfies the freshness condition at time T for some freshness interval t.` ( (T − Tc) < t || T_client == T_server )`

```latex
Tc: last time the entry was validated by client. 
T_client: last time that the file was modified by client.
T_server: last time that the file was modified by the server.
```

I have followed strcitly as described on the A3-spec and will not give more details about exactly how to check freshness as those are exact sentences on the spec. So for my implementation, freshness is all checked at the time of client's `getattr, read, write, utimens and truncate`

When a fsync is issued, the client's file will be uploaded to server. All time metadata are updated accordingly. The last acces time of a file (saved in `fileMetadata` structure) is set to current time after open or freshness check.



#### 2. Error Codes

```c
#define LOCKERR -600;
#define UNLOCKERR - 700;
```

Those are returned when a call is failed to get the lock or fail to unlock. 

For other error codes, `-EMFILE` is returned when mutiple files are required to access/open a single file. `-errorno` is returned for any system call failure. 



#### 3. Testings

I have implemented multiple python function to do the file manipulation and test each piece of my client-server model implementation. 

`basic_open_check.py`: create new and open file with multiple flags for read/write flag. Try to write to a file under different open scenarios and see if error returned. Then close the file. Also test for upload.

`basic_upload_truncate.py` : open existing file on local and truncate it and then close the file.

`basic_fsync_utimens_check.py`: (1) issue fsync after open and write operations. (2) issue utimens and check for file metadata.

`complex_mutual_exclusion.py`: open two client and trigger write operations. See if there is error.

`complex_freshness_check.py`:do the normal read and write but including a sleep function for 10s



Note that there are more testing scripts that I have implemented but to keep the manual concise, i would not list all of them. But from above, clearly, you can see that I test the model from two different approaches/perspective: (1) do a series of file system call from single client. (2) do complex calls from multile clients.

---



