# About
Frog combines the power of Tesseract, PaddleOCR, and other open software in a single application where you can 
configure your pipeline with custom profiles and database integration. Options are currently limited, but these 
stages are available right now:
 - Tesseract Text Recognition
 - Paddle Text Detection
 - Paddle Text Angle Classification (0/180 deg)

Each stage has its own input and output based on which type it is.
- Text Detection
   - Input: Full image
   - Output: List of quads
- Text Recognition
  - Input: Full image, list of quads
  - Output: Frog OCR Document
- Text Angle Classification
  - Input: Full image, list of quads
  - Output: List of angle classifications (angle and confidence)

## Installation
1. Install PostgreSQL and other dependencies.
    ```shell
    apt install postgresql libjpeg8 libopenjp2-7 libtiff5
    ```
2. Create a new database to store the incoming tasks.
   ```sql
   create database frog;
   create user frog with encrypted password 'frog';
   grant all privileges on database frog to frog;
   ```
3. Execute `Resources/database.pg.sql` to install the database.
4. Save default configuration to `/etc/frog/config.xml`, and edit as needed.
   ```shell
   frog config
   ```

### Set up as a service
Move the service configuration to `/etc/frog/frog.service`.
```shell
systemctl link /etc/frog/frog.service
systemctl daemon-reload
systemctl enable frog
systemctl start frog
```

## Commands
- help: Get more information about the different commands.
- add: Add new tasks.
- process: Process tasks.
- validate: Validate output files.
- config: Install default configuration.
