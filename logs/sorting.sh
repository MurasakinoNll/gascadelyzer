#!/bin/bash

# Function to remove timestamps in the format s.mmm to sss.mmm
remove_timestamps() {
  awk '
  {
    # Check if the first field matches the timestamp format (1 to 3 digits, a dot, and 3 more digits)
    if ($1 ~ /^[0-9]{1,3}\.[0-9]{3}$/) {
      # Remove the timestamp
      $1 = "";
      sub(/^[[:space:]]+/, "");  # Remove leading spaces
    }
    print
  }' "$1" > "$2"
}

# Remove timestamps from both files
remove_timestamps spawned2 file1_no_timestamps.txt
remove_timestamps cancelable.log file2_no_timestamps.txt

# Find lines unique to file1 and file2, preserving order
grep -Fvx -f file2_no_timestamps.txt file1_no_timestamps.txt > unique_to_file1.txt
grep -Fvx -f file1_no_timestamps.txt file2_no_timestamps.txt > unique_to_file2.txt

# Combine the unique lines from both files
cat unique_to_file1.txt unique_to_file2.txt > diff.txt

# Remove duplicate lines while preserving order
awk '!seen[$0]++' diff.txt > final_diff.txt

# Clean up temporary files
rm file1_no_timestamps.txt file2_no_timestamps.txt unique_to_file1.txt unique_to_file2.txt diff.txt

