
def html_to_header(input_file, output_file, array_name):
    with open(input_file, 'r') as f:
        content = f.read()

    with open(output_file, 'w') as f:
        f.write('#include <Arduino.h>\n')
        f.write('#pragma once\n\n')
        f.write('const char {}[] PROGMEM = R"rawliteral(\n'.format(array_name))
        f.write(content)
        f.write('\n)rawliteral";\n')

html_to_header('src/index.html', 'src/index_html.h', 'index_html')
html_to_header('src/addressList.html', 'src/addressList.h', 'addressList')

