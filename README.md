# react-template-cli (rtc) 

### Requirements
- cmake standard >= 3.25

### Installation
```bash
git clone git@github.com:yannickbohrer/react-template-cli.git
cd react-template-cli
sudo cmake -S ./ -B .build/
cd .build
make
sudo make install
```

### Syntax 
`rtc [activity] [type] [path] [--flags]`

### Usage
`[]` indicate optional parameter
- `rtc generate component path/to/MyComponent [--css] [--template=CustomTemplate]`
- `rtc add template path/to/MyExistingFile`
- `rtc remove template MyTemplate`
- `rtc list template`
- `rtc history`

Flags:
- `--css`: Dedicated css file for component will be generated
- `--template=CustomTemplate`: Use custom template for component generation. Not set: Standard template will be used

### Important:
- when adding a template file, the file name (without file extension) has to match component name exactly

#### Assets Directory:
```bash
~/.rtc/
```
