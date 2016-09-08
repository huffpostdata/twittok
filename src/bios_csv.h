#ifndef BIOS_CSV_H
#define BIOS_CSV_H

class BiosCsv {
  struct Entry {
    const char* id;
    size_t id_len;
    bool follows_clinton;
    bool follows_trump;
    const char* bio;
    size_t bio_len;
}

#endif /* BIOS_CSV_H */
