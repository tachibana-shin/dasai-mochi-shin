import wave
import sys
import os
import subprocess
import io
import argparse

def audio_to_pcm_header(input_path, header_path, sample_rate=None):
    # This will convert any audio (mp3, wav, etc.) to 16-bit PCM WAV
    try:
        # -ar sets the audio sampling frequency
        # -ac 1 force to mono if needed? (Current project seems to support mono/stereo)
        # Let's keep it as is, or maybe add channel option if requested later.
        cmd = ['ffmpeg', '-i', input_path]
        if sample_rate:
            cmd += ['-ar', str(sample_rate)]
        cmd += ['-f', 'wav', '-acodec', 'pcm_s16le', '-']
        
        process = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        stdout, stderr = process.communicate()
        
        if process.returncode != 0:
            print(f"Error: FFmpeg failed to process {input_path}")
            print(stderr.decode())
            return

        wav_data = io.BytesIO(stdout)
    except FileNotFoundError:
        print("Error: FFmpeg not found. Please install FFmpeg and add it to your PATH.")
        return

    with wave.open(wav_data, 'rb') as wav_file:
        params = wav_file.getparams()
        nchannels, sampwidth, framerate, nframes, comptype, compname = params
        
        frames = wav_file.readframes(nframes)
        
        # Get variable name from filename
        base_name = os.path.basename(input_path).rsplit('.', 1)[0]
        var_name = base_name.replace('-', '_').replace(' ', '_').replace('.', '_')
        
        with open(header_path, 'w') as f:
            f.write(f"#ifndef {var_name.upper()}_H\n")
            f.write(f"#define {var_name.upper()}_H\n\n")
            f.write(f"#include <Arduino.h>\n\n")
            f.write(f"// Audio info: {framerate}Hz, {nchannels} channel(s)\n")
            f.write(f"const uint32_t {var_name}_sample_rate = {framerate};\n")
            f.write(f"const uint32_t {var_name}_size = {len(frames)};\n")
            f.write(f"const uint8_t {var_name}_data[] PROGMEM = " + "{\n")
            
            for i in range(0, len(frames), 12):
                chunk = frames[i:i+12]
                f.write("  " + ", ".join([f"0x{b:02x}" for b in chunk]) + ",\n")
            
            f.write("};\n\n")
            f.write(f"#endif // {var_name.upper()}_H\n")
            
    print(f"Converted {input_path} to {header_path} at {framerate}Hz")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Convert audio files to PCM C headers.')
    parser.add_argument('input', help='Input audio file (wav, mp3, etc.)')
    parser.add_argument('output', help='Output .h file')
    parser.add_argument('--rate', '-r', type=int, help='Sample rate (default: keep original)')

    args = parser.parse_args()
    audio_to_pcm_header(args.input, args.output, args.rate)
