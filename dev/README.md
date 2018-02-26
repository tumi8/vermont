# VERMONT VM

This vagrant setup provides an entry point for directly testing vermont without
further setup on a local machine.

It creates a VM which automatically installs all necessary dependecies for
vermont and builds the currently checked out version of it.
The repository is available in the VM via /vermont

## Setup

Install Vagrant and Ansible each with version >= 2.0.
Then, in this folder

``` shell
$ vagrant up
...wait...
$ vagrant ssh
# cd /vermont
# ./vermont -f <config-file>
```
## Build Configuration

The build process, its steps and parameters  can be configured in
roles/base/tasks/main.yml
