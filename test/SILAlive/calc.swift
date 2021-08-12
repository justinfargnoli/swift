if CommandLine.arguments.count != 3 {
	print("Usage: $ main <int> <op> <int>")
	print("Ignore all further results.")
}

let operand1 = Int(CommandLine.arguments[0])!
let operand2 = Int(CommandLine.arguments[2])!

let op = CommandLine.arguments[1]

switch op {
case "+":
	print(operand1 + operand2)
case "-":
	print(operand1 - operand2)
case "*":
	print(operand1 * operand2)
case "/":
	if (operand2 == 0) {
		print("ERROR: operand2 being 0 causes a division by 0 error")
	}
  print(operand1 / operand2)
default:
	print("ERROR: \"\(op)\" is not a supported operator")
}
