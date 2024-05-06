#!/usr/bin/python3
import pretty_midi
import serial
import time

# Initialize a new PrettyMIDI object
midi_file = pretty_midi.PrettyMIDI()

# Create a piano instrument
piano_program = pretty_midi.instrument_name_to_program('Acoustic Grand Piano')
piano = pretty_midi.Instrument(program=piano_program)
# Add the piano instrument to the MIDI file
midi_file.instruments.append(piano)

# Initialize the serial connection
ser = serial.Serial('/dev/ttyS6', baudrate=9600, timeout=1)

# Initialize previous timestamp in ms(currently None), the timeout for ending reading without any notes (5 seconds)
# and the last note time in ms(current time * 1000)
previous_timestamp_ms = None
no_note_timeout = 5
last_note_time_ms = time.time() * 1000

try:
    while True:
        try:
            # Read a line from serial
            line = ser.readline().decode().strip()
            
            if line:
                print("Received: ", line)
                # Reset the no note timer since a note has been sensed
                last_note_time_ms = time.time() * 1000

                # Parse the received data ("timestamp,pitch")
                timestamp_ms, pitch_str = line.split(',')
                timestamp_ms = int(timestamp_ms)
                pitch = pretty_midi.note_name_to_number(pitch_str.strip())

                # Start time is just the timestamp given
                note_start = timestamp_ms / 1000.0

                # Calculate duration based on the timestamp of the current note and the previous note
                if previous_timestamp_ms is not None:
                    duration = (timestamp_ms - previous_timestamp_ms) / 1000.0
                    previous_timestamp_ms = timestamp_ms
                else:
                    # This is realtime pitch detection, so we'll have to eliminate
                    # the first note due to limitations. It won't be too noticeable
                    duration = 0.0
                    previous_timestamp_ms = timestamp_ms

                # Create a note with the extracted pitch, start time, and calculated duration
                note = pretty_midi.Note(velocity=100, pitch=pitch, start=note_start, end=note_start + duration)
                piano.notes.append(note)

        except serial.SerialException:
            # Handle serial communication error
            print("Serial communication error. Exiting program.")
            break

        # Check if timeout has been reached
        if (time.time() * 1000) - last_note_time_ms > no_note_timeout * 1000:
            print("No notes sensed for {} seconds. Exiting program.".format(no_note_timeout))
            break
        
# Close the serial port if interrupted so we can end execution
except KeyboardInterrupt:
    ser.close()

# Save the MIDI file
midi_file.write('output_file.mid')