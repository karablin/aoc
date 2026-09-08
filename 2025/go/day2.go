package main

import (
	"fmt"
	"io"
	"os"
	"strconv"
	"strings"
)

func main() {
	answer1, answer2 := 0, 0

	input, _ := io.ReadAll(os.Stdin)

	for _, numRange := range strings.Split(string(input), ",") {
		numLowHi := strings.Split(numRange, "-")
		if len(numLowHi) != 2 {
			fmt.Fprintln(os.Stderr, "not a XX-YY pair:", numLowHi)
			return
		}

		low, err1 := strconv.Atoi(strings.TrimSpace(numLowHi[0]))
		high, err2 := strconv.Atoi(strings.TrimSpace(numLowHi[1]))
		if err1 != nil || err2 != nil {
			fmt.Fprintln(os.Stderr, "cannot convert", numLowHi, "to number pairs")
			return
		}

		for id := low; id <= high; id++ {
			idStr := strconv.Itoa(id)
			idLen := len(idStr)

			// part 1:
			if idLen%2 == 0 && idStr[:idLen/2] == idStr[idLen/2:] {
				answer1 += id
			}
			// part 2: process number for each integer number of groups (e.g. for 1234567890 - group count will be 2,5, and 10)
			for numGroups := 2; numGroups <= idLen; numGroups++ {
				if idLen%numGroups != 0 {
					continue
				}
				// compare groups in pairs, from first to last-1
				// if all pairs is equal, then ID is fake
				allGroupsEqual := true
				groupLen := idLen / numGroups
				for start := 0; start < (numGroups-1)*groupLen; start += groupLen {
					mid := start + groupLen
					end := mid + groupLen
					if idStr[start:mid] != idStr[mid:end] {
						allGroupsEqual = false
						break
					}
				}
				if allGroupsEqual {
					answer2 += id
					break
				}
			}
		}
	}

	fmt.Println("Answer 1:", answer1)
	fmt.Println("Answer 2:", answer2)
}
