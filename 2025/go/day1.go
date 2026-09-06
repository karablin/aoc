package main

import (
	"bufio"
	"fmt"
	"os"
	"strconv"
)

func main() {
	scanner := bufio.NewScanner(os.Stdin)
	answer1, answer2 := 0, 0
	x := 50
	for scanner.Scan() {
		line := scanner.Text()

		dir := 1
		if line[0] == 'L' {
			dir = -1
		}

		num, err := strconv.Atoi(line[1:])
		if err != nil {
			fmt.Println(err)
			return
		}

		oldX := x
		delta := num * dir
		fullCircles := num / 100
		remainder := delta % 100

		x = ((x+delta)%100 + 100) % 100
		// first part of puzzle
		if x == 0 {
			answer1++
		}
		// each full circle passes through 0
		answer2 += fullCircles
		// check if non-full circle remainder passed through 0/+100
		passedThroughZero := oldX+remainder >= 100 || (oldX > 0 && oldX+remainder <= 0)
		if passedThroughZero {
			answer2++
		}
	}
	if err := scanner.Err(); err != nil {
		fmt.Fprintln(os.Stderr, "reading standard input:", err)
	}
	fmt.Println("Answer 1:", answer1)
	fmt.Println("Answer 2:", answer2)
}
