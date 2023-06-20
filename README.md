# FrogOCR
Turn images into text with Tesseract, and output it as AltoXML.

## Installation
1. Install PostgreSQL and other dependencies.
    ```shell
    apt install postgresql
    apt install libjpeg8 libopenjp2-7 libtiff5
    ```
2. Create new PostgreSQL database and user with all privileges.
   ```sql
   create database frog;
   create user frog with encrypted password 'frog';
   grant all privileges on database frog to frog;
   ```
3. Create the default configuration;
   ```shell
   frog --install
   ```
4. Edit `/etc/Frog/Configuration.xml` if necessary.
5. Finish creating the database:
   ```shell
   frog --create-database 
   ```

### Setting up the FrogOCR service
Move the service configuration to `/etc/Frog/frog.service`.
```shell
systemctl link /etc/Frog/frog.service
systemctl daemon-reload
systemctl enable frog
systemctl start frog
```

### What now?
Add tasks to the `Task` table. Prefer creating a new user for adding tasks. If FrogOCR is running, it will periodically check for new tasks.
There might be more configurations later, but for now these are hardcoded values:
```
Max tasks per thread = 100

Minutes to sleep if database connection failed = 5

Minutes to sleep if queue is empty = 10

Milliseconds to sleep if all threads have max tasks = 500
```

## Options
### --validate [path]
Validate AltoXML files. Path can be a file or directory. Note that every XML file will be validated in a directory.

### --exit-if-no-tasks
Exit instead of sleeping when there are no tasks to process.

### --install
Install default configuration in `/etc/Frog/Configuration.xml`.

### --create-database
Creates the Task table in the configured database.

### --version
Prints the FrogOCR and Tesseract versions.

### --help
