# Copyright (c) 2024 Mirror-Touch Media

#
# This Python script will iterate over local cache files in your Unreal project and delete them.
# We often find this useful for troubleshooting, or for running Perforce reconciliation with fewer steps (since we don't want to push cache files).
# 
# For ease of scheduling this script and ad-hoc by non-programmers, there is a .bat file included that will run the Python script and print progress/errors to the terminal.
# 
# IMPORTANT: This script is currently designed to work specifically from an Automations folder at the top-level of an Unreal Engine project.
#
# If you want to change which directories/files are included for deletion, just modify the dirs_to_delete and patterns_to_match lists.
# Notice that we also walk through any Plugins directories in the project to delete their local cache files, as well.
#
# Note that 1) we use JetBrains IDEs, and 2) we don't push our IDE files to the depot, hence the inclusion of Rider/PyCharm-related directories/files.
# 


import os
import shutil


# DEFINE THE DIRECTORIES AND FILES TO DELETE
# ------------------------------------------

# Determine the parent directory of the current script
parent_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))

# Define top-level directories to delete. Modify as needed.
dirs_to_delete = [
    os.path.join(parent_dir, ".idea"),
    os.path.join(parent_dir, ".vs"),
    os.path.join(parent_dir, "Binaries"),
    os.path.join(parent_dir, "Build"),
    os.path.join(parent_dir, "DerivedDataCache"),
    os.path.join(parent_dir, "Intermediate"),
    os.path.join(parent_dir, "Saved")
]

# Recursively find Binaries/Intermediate directories under "Plugins" and add them to the list. Modify as needed.
for root, dirs, files in os.walk(os.path.join(parent_dir, "Plugins")):
    for dir_name in dirs:
        plugins_dir = os.path.join(root, dir_name)
        dirs_to_delete.extend([
            os.path.join(plugins_dir, "Binaries"),
            os.path.join(plugins_dir, "Intermediate")
        ])


# Define top-level file extensions or names to match for deletion. Modify as needed.
patterns_to_match = [
    ".vsconfig",
    ".sln",
    ".DotSettings.user"
]

# Create a list of file paths to delete based on the above extension/name checks.
files_to_delete = []
for file_name in os.listdir(parent_dir):
    if any(file_name.endswith(pattern) for pattern in patterns_to_match):
        files_to_delete.append(os.path.join(parent_dir, file_name))



# DEFINE DELETION METHODS
# ------------------------------------------

# Function to delete a directory and its contents
def delete_directory(directory):
    if os.path.exists(directory):
        print(f"Deleting directory: {directory}")
        shutil.rmtree(directory)

# Function to delete a file
def delete_file(file_path):
    if os.path.exists(file_path):
        print(f"Deleting file: {file_path}")
        os.chmod(file_path, 0o777)
        os.remove(file_path)



# EXECUTION DELETION OF DIRECTORIES AND FLIES
# ------------------------------------------

# Delete directories
for dir_path in dirs_to_delete:
    delete_directory(dir_path)

# Delete files
for file_path in files_to_delete:
    delete_file(file_path)

print("Cleanup completed.")
