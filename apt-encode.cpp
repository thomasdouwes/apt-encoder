// apt-encode: Encode images as APT audio signals
// Copyright (C) 2020  Gokberk Yaltirakli
//
// This program is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License as published by the Free
// Software Foundation, either version 3 of the License, or (at your option) any
// later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public License for more
// details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

// Utils

template <typename F, typename T>
constexpr T map(F value, F f1, F t1, T f2, T t2) {
  return f2 + ((t2 - f2) * (value - f1)) / (t1 - f1);
}

template <typename T> constexpr T max(T val1, T val2) {
  return val1 > val2 ? val1 : val2;
}

// Constants and config

constexpr size_t CARRIER = 2400;
constexpr size_t BAUD = 4160;
constexpr size_t OVERSAMPLE = 3;

// Sync words for the left and right images

constexpr const char *SYNCA = "000011001100110011001100110011000000000";
constexpr const char *SYNCB = "000011100111001110011100111001110011100";

// Image

class Image {
public:
  Image(const char *);
  void free();

  uint8_t getPixel(size_t, size_t) const;

  size_t width() const;
  size_t height() const;

  void load();

private:
  void skipComment();

  FILE *m_file;
  size_t m_height;
  uint8_t *m_pixels;
};

Image::Image(const char *path) { m_file = fopen(path, "r"); }

void Image::load() {
  skipComment();

  // Read the file type: P2 + newline
  {
    char buf[3];
    fread(buf, 1, 3, m_file);
    assert(buf[0] == 'P');
    assert(buf[1] == '2');
    assert(buf[2] == '\n');
  }
  skipComment();

  size_t maxValue, width;

  // TODO: Support resizing images using a simple algoritm
  fscanf(m_file, "%zu", &width);
  assert(width == 909 && "Images should have a width of 909");
  skipComment();

  fscanf(m_file, "%zu", &m_height);
  skipComment();

  // TODO: Use maxValue to normalize colours
  fscanf(m_file, "%zu", &maxValue);
  skipComment();

  m_pixels = (uint8_t *)malloc(width * m_height * sizeof(uint8_t));

  for (size_t i = 0; i < m_height * width; i++) {
    fscanf(m_file, "%hhu", &m_pixels[i]);
    skipComment();
  }

  fclose(m_file);
}

void Image::skipComment() {
  // Check if this is the beginning of a comment. If the current character is
  // #, read until we reach a newline.
  int ch = fgetc(m_file);

  if (ch != '#') {
    ungetc(ch, m_file);
  } else {
    while (fgetc(m_file) != '\n')
      ;
  }
}

void Image::free() { std::free(m_pixels); }

size_t Image::width() const { return 909; }

size_t Image::height() const { return m_height; }

uint8_t Image::getPixel(size_t x, size_t y) const {
  return m_pixels[y * width() + x];
}

// Audio

void write_value(uint8_t value) {
  static double sn = 0;

  for (size_t i = 0; i < OVERSAMPLE; i++) {
    double samp = sin(CARRIER * 2.0 * M_PI * (sn / (BAUD * OVERSAMPLE)));
    samp *= map((int)value, 0, 255, 0.0, 0.7);

    uint8_t buf[1];
    buf[0] = map(samp, -1.0, 1.0, 0, 255);
    fwrite(buf, 1, 1, stdout);

    sn++;
  }
}

int main(int argc, char **argv) {
  // TODO: Improve command line argument parsing

  // If there are no arguments, print usage and return.
  if (argc < 2) {
    printf("Usage: %s ./image1.pgm ./image2.pgm\n", argv[0]);
    return 1;
  }

  // If there are two images, use them as the left and right images
  // respectively, otherwise use the same image for both channels.

  std::vector<Image> images;
  for(int i=1; i<argc; i++)
  {
	images.push_back(Image(argv[i]));
	images[i-1].load();
  }

  struct t_channel_plan
  {
 	int start_frame; // frame to start using this channel
	int channel_a; // AVHRR channel from 1-6 for APT channel A
	int image_a; // which image to use for APT channel B
	int space_a; // value for space A
	int channel_b; // AVHRR channel from 1-6 for APT channel B
	int image_b; // which image to use for APT channel B
	int space_b; // value for space B
  };



  // struct t_channel_plan channel_plan[4] = {
  //   {0, 3, 2, 255, 4, 1, 255},
  //   {16, 1, 0, 0, 4, 1, 255},
  //   {30, 3, 2, 255, 4, 1, 255},
  // };

  struct t_channel_plan channel_plan[1] = {
    {0, 2, 0, 0, 4, 1, 255},
  };

  int channel_plans = sizeof(channel_plan) / sizeof(struct t_channel_plan);

  // Telemetry block A
  int telem_a[16] = {
	32, // WEDGE #1
	64, // WEDGE #2
	96, // WEDGE #3
	128, // WEDGE #4
	160, // WEDGE #5
	192, // WEDGE #6
	224, // WEDGE #7
	255, // WEDGE #8
	0, // Zero Modulation Reference
	32, // Thermistor Temp. #1
	32, // Thermistor Temp. #2
	32, // Thermistor Temp. #3
	32, // Thermistor Temp. #4
	96, // Patch Temp.
	0, // Back Scan
	0 // Channel I.D. Wedge (defined in channel plan)
  };

  // Telemetry block B
  int telem_b[16] = {
	32, // WEDGE #1
	64, // WEDGE #2
	96, // WEDGE #3
	128, // WEDGE #4
	160, // WEDGE #5
	192, // WEDGE #6
	224, // WEDGE #7
	255, // WEDGE #8
	0, // Zero Modulation Reference
	32, // Thermistor Temp. #1
	32, // Thermistor Temp. #2
	32, // Thermistor Temp. #3
	32, // Thermistor Temp. #4
	96, // Patch Temp.
	96, // Back Scan
	0 // Channel I.D. Wedge (defined in channel plan)
  };

  // Space A value
  int space_a_value; // (defined in channel plan)
  // Space B value
  int space_b_value; // (defined in channel plan)

  Image* cha_image;
  Image* chb_image;

  int marker[4] = {0, 0, 255, 255};

  int channel_plan_current = 0;

  auto height = images[0].height();

  for (size_t line = 0; line < height; line++) {
    // channel selection
    int frame = line / 128;
    int frame_line = line % 128;
    if(!frame_line)
    {
        for(int i=0; i < channel_plans; i++)
        {
            if(channel_plan[i].start_frame <= frame)
            {
                channel_plan_current = i;
                break;
            }
        }
	
	telem_a[15] = channel_plan[channel_plan_current].channel_a * 32;
	telem_b[15] = channel_plan[channel_plan_current].channel_b * 32;
	space_a_value = channel_plan[channel_plan_current].space_a;
	space_b_value = channel_plan[channel_plan_current].space_b;
	cha_image = &images[channel_plan[channel_plan_current].image_a];
	chb_image = &images[channel_plan[channel_plan_current].image_b];
    }

    // marker
    if(line % 120 < 4)
    {
	space_a_value = marker[line % 120];
	space_b_value = marker[line % 120];
    }
    if(line % 120 == 4) // very hacky
    {
	space_a_value = channel_plan[channel_plan_current].space_a;
	space_b_value = channel_plan[channel_plan_current].space_b;
    }

    // Sync A
    for (size_t i = 0; i < strlen(SYNCA); i++)
      write_value(SYNCA[i] == '0' ? 0 : 255);

    // Space A
    for (size_t i = 0; i < 47; i++)
        write_value(space_a_value);

    // Image A
    for (size_t i = 0; i < cha_image->width(); i++) {
      if (line < cha_image->height())
        write_value(cha_image->getPixel(i, line));
      else
        write_value(0);
    }

    // Telemetry A
    size_t wedge = frame_line / 8;
    auto v = telem_a[wedge];
    for (size_t i = 0; i < 45; i++) {
      write_value(v);
    }

    // Sync B
    for (size_t i = 0; i < strlen(SYNCB); i++)
      write_value(SYNCB[i] == '0' ? 0 : 255);

    // Space B
    for (size_t i = 0; i < 47; i++)
        write_value(space_b_value);

    // Image B
    for (size_t i = 0; i < chb_image->width(); i++) {
      if (line < chb_image->height())
        write_value(chb_image->getPixel(i, line));
      else
        write_value(0);
    }

    // Telemetry B
    for (size_t i = 0; i < 45; i++) {
      size_t wedge = frame_line / 8;
      auto v = telem_b[wedge];
      write_value(v);
    }
  }

  for(int i=1; i<argc; i++)
  {
	images[i-1].free();
  }

  return 0;
}
