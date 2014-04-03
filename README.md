
# Resow - Pluggable Web Services for C

Resow is a project that realizes modularized web development
for C developers.

This project is still in early development stage and constitutes
no more than an idea how to implement web services in C in a
nice and easy way.

## Prerequisites

* Apache 2.x
* MySQL 5.x (client libs)

A fully configured MySQL-Server is needed, too to make Resow
work. See config.h for installation parameters.

## Installation

### Makefile configuration

Check the installation paths. You will need to setup everything
properly because Resow is using the paths in the compiled binary
to find all the pieces of the software.

### Configuration in config.h

Please take a look at config.h in the project top directory. There
are some important parameters to configure.

The database credentials do not need to be changed if you secure access
to your server properly.

### Compiling

```
# make
```

### Installation

```
# su root
# make install
```

This will install resow.

### For flatfs

Create the directory, you specified in config.h and setup the proper permissions
so Resow is able to write there.

### Database initialization

#### MySQL

Create a user and grant access to resow database:
```
# cd db_schema/mysql
# mysql -u root -p
mysql> GRANT ALL PRIVILEGES ON resow.* TO 'resow'@'localhost' IDENTIFIED BY 'resow';
mysql> CREATE DATABASE resow CHARACTER SET = UTF8;
mysql> \. init.sql
```

The string in quotes after *IDENTIFIED BY* is the database password.

### Apache integration

*TODO*

# Security notices

Please be careful not to allow people to download the resow binary. A misconfiguration
can reveal database passwords, because they are stored within the binary.
