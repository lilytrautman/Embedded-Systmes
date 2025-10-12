import serial
import argparse
import time
import json


DEFAULT_PORT = "COM6"
DEFAULT_BAUD = 115200
DEFAULT_TIMEOUT = 1
DEFAULT_DELAY = 0.5
DEFAULT_TEST_TYPE = "config"


def additional_test(port, baud, test_file):
    ser = serial.Serial(
        port, baud, timeout=DEFAULT_TIMEOUT, stopbits=serial.STOPBITS_ONE
    )
    # wait for connection to be established
    time.sleep(2)

    # load json test
    with open(test_file, "r") as f:
        tests = json.load(f)

    for idx, test in enumerate(tests["additional_test"], start=1):
        input_msg, expected = test
        input_str = json.dumps(input_msg)

        ser.write((input_str + "\n").encode("utf-8"))
        resp_line = ser.readline().decode("utf-8").strip()

        try:
            resp = json.loads(resp_line)
        except json.JSONDecodeError:
            print(f"[Test {idx}] FAIL: Received invalid JSON response -> {resp_line}")
            time.sleep(DEFAULT_DELAY)
            continue

        if resp == expected:
            print(f"[Test {idx}] PASS: Response matches expected output.")
        else:
            print(f"[Test {idx}] FAIL:")
            print("  - Sent:")
            print(json.dumps(input_msg, indent=2))
            print("  - Expected:")
            print(json.dumps(expected, indent=2))
            print("  - Received:")
            print(json.dumps(resp, indent=2))
            print()
        time.sleep(DEFAULT_DELAY)

    ser.close()


def setup_config(port, baud, test_file):
    ser = serial.Serial(
        port, baud, timeout=DEFAULT_TIMEOUT, stopbits=serial.STOPBITS_ONE
    )
    # wait for connection to be established
    time.sleep(2)

    # load json test
    with open(test_file, "r") as f:
        tests = json.load(f)

    for idx, test in enumerate(tests["config_tests"], start=1):
        input_msg, expected = test
        input_str = json.dumps(input_msg)

        ser.write((input_str + "\n").encode("utf-8"))
        resp_line = ser.readline().decode("utf-8").strip()

        try:
            resp = json.loads(resp_line)
        except json.JSONDecodeError:
            print(f"[Test {idx}] FAIL: Received invalid JSON response -> {resp_line}")
            time.sleep(DEFAULT_DELAY)
            continue

        if resp == expected:
            print(f"[Test {idx}] PASS: Response matches expected output.")
        else:
            print(f"[Test {idx}] FAIL:")
            print("  - Sent:")
            print(json.dumps(input_msg, indent=2))
            print("  - Expected:")
            print(json.dumps(expected, indent=2))
            print("  - Received:")
            print(json.dumps(resp, indent=2))
            print()
        time.sleep(DEFAULT_DELAY)

    ser.close()


def main(port, baud):
    print("=" * 40)
    print("Running CONFIG tests")
    print("=" * 40)
    setup_config(port, baud, "config.json")
    print("\n" + "=" * 40)
    print("Running ADDITIONAL tests")
    print("=" * 40)
    additional_test(port, baud, "additional_test.json")
    print("\nFinished setting up config.")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Run config tests for the embedded IR camera system."
    )
    parser.add_argument(
        "--port",
        type=str,
        default=DEFAULT_PORT,
        help="Serial port to use (default: COM6)",
    )
    parser.add_argument(
        "--baud",
        type=int,
        default=DEFAULT_BAUD,
        help="Baud rate to use (default: 115200)",
    )
    args = parser.parse_args()
    main(args.port, args.baud)
