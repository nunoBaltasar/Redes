#ifndef VALIDATION_H
#define VALIDATION_H

#include <string>

// Regras da secção "Validation and Limits" do enunciado

bool validate_uid(const std::string& uid);            // exatamente 6 dígitos
bool validate_password(const std::string& pw);        // exatamente 8 alfanuméricos
bool validate_port(const std::string& port);          // inteiro decimal 1..65535
bool validate_filename(const std::string& filename);  // <= 24 chars, [A-Za-z0-9_-]+ "." 3 alfanuméricos
bool validate_label(const std::string& label);        // 1..20 chars de [A-Za-z0-9_-]
bool validate_fsize(long long fsize);                 // 0..10 000 000 bytes

#endif
