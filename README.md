# kvdb - A simple key-value database

This is a **key-value NoSQL database** written focusing on performance and storage efficiency.\
The database can be accessed by multiple processes concurrently.


### Usage

1. Build the project: `$ make`
2. Run the daemon: `$ ./kvdbd`
3. Run the CLI with arguments to interact with the daemon: `$ ./kvdb set item val`


### Functionalities

The following functionalities have been implemented:

* `kvdb set {key} {value}` : Associates `key` with `value`.
* `kvdb get {key}` : Fetches the value associated with `key`.
* `kvdb del {key}` : Removes `key` from the database.
* `kvdb ts {key}` : Returns the timestamp when `key` was first set and when it was last set in the format "YYYY-MM-DD HH:MM:SS.SSS".


## Architecture

The system is composed of two parts:

* **kvdb:** Client providing a CLI with the commands to interact with the core.
* **kvdbd:** Daemon implementing the core functionalities and managing the data store.


## Key features

* The DB is stored in memory with a **hash table** at its core.
* Concurrent access is handled with **POSIX Threads**: each request results in the creation of a new thread. Multithreading safety is achieved by **RW locks**, which allow for both data consistency and efficiency in concurrent access (multiple readers, single writer).
* The CLI and the daemon interact through **UNIX domain sockets** (inter-process communication sockets).

---

## Limitations

* It can memorize only strings as values, more structured data is not supported.
* It lives in RAM, so any critical issue with the machine running the DB would result in data loss.
* The Hash Table with which the DB is built at its core implements separate chaining as the strategy to handle collisions, and no rehashing is implemented. In this case, the more entries are in the DB (higher load factor), the more the performance would be affected.
* The daemon does not provide a way to quit and terminate itself gracefully (e.g. without Ctrl-C).
* The code could be better structured for readability and maintainability.
* Error handling could be done better (in my code I take for granted that all the operations succeed, or that input commands are always good).
* At the start of the daemon, I initialize all the locks inside a cycle. This is not an optimal solution, as I could have written the init of a single lock just when it was needed. To do that, I should have made a struct for every cell of the hash table containing also its own lock. I thought about it too late in the development. I could not simply add a lock into the struct for the list nodes because the locking mechanism is intended to work on the cells of the HT and not on the single nodes.
* From the point of view of the code, there could be many improvements, especially for readability.

### Potential improvements:

* A more fault-tolerant implementation could be achieved by saving data to disk.
* A more efficient hash table (suitable for this small case study) could be achieved by implementing an open addressing technique (such as double hashing) for handling the collisions. By having all the elements in the same table, the cache could be better exploited.
