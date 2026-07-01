# VBIT2 control interface

The VBIT2 control interface is a TCP socket server for the insertion of data broadcast packet data, modification of service settings, and dynamic management of pages.

*This document describes version 1.1.0 of the interface API.*

The server supports up to five simultaneous connections, and uses a variable length binary message format.
Clients send commands to the server, which will return a response containing an error/status code, and any data requested by the client.

The first byte of every command/response is a message length byte. This is the total length of the message including the message length byte.

The second byte of a command message selects a command. The following command bytes are defined:
|Byte | Mnemonic  | Description                                 |
|-----|-----------|---------------------------------------------|
|`&00`|`SETCHAN`  | Set channel number for subsequent commands. |
|`&01`|`DBCASTAPI`| Data broadcast API command.                 |
|`&02`|`CONFIGAPI`| Service configuration API command.          |
|`&03`|`PAGESAPI` | Page data API command.                      |
|`&04`|`GETAPIVER`| Get API version the server implements.      |

Undefined commands return `CMDERR`.

The remainder of each command has a variable length depending on the command arguments.
Each API command is followed by sub-command numbers specific to those interfaces.

The second byte of a response message is a status/error code:
|Byte | Mnemonic | Description                                           |
|-----|----------|-------------------------------------------------------|
|`&00`|`CMDOK   `| Command completed successfully.                       |
|`&FC`|`CMDTRUNC`| Command completed but data was truncated.             |
|`&FD`|`CMDNOENT`| Command failed because requested entity doesn't exist.|
|`&FE`|`CMDBUSY `| Command failed because operation is blocked.          |
|`&FF`|`CMDERR  `| Command failed with a general error.                  |

### SETCHAN - Set data channel - version 1.0.0 up:
This command takes a single byte which selects a channel number from 0 to 15.
Channel 0 supports multiple simultaneous connections, but the 15 databroadcast channels may each only have a single client connected at a time.

    byte:      0       1        2
    value: [  &03 ][  &00  ][  0-15 ]
           (length)(SETCHAN)(channel)

Possible error/status values:
| Code     | Reason                                   |
|----------|------------------------------------------|
|`CMDOK`   | Channel set successfully.                |
|`CMDBUSY` | Channel is blocked by another connection.|
|`CMDERR`  | Invalid channel number or command length.|

### DBCASTAPI - Databroadcast command - version 1.0.0 up:
 The third byte selects a sub-command. The following sub-command bytes are defined:
|Byte | Mnemonic  | Description                                     |
|-----|-----------|-------------------------------------------------|
|`&00`|`DCRAW`    | Push a raw packet data to data broadcast buffer.|
|`&01`|`DCFORMATA`| Push a format A data broadcast packet to buffer.|
|`&02`|`DCFORMATB`| Load a format B data broadcast payload bundle.  |
  
Undefined sub-commands return `CMDERR`.
`DBCASTAPI` commands are only valid for channels 1-15.

#### DCRAW - Push a raw packet data to data broadcast buffer - version 1.0.0 up:
This command inserts a 40 byte packet into a data broadcast transmission buffer.
The command requires exactly 40 bytes of packet data which will be transmitted unmodified.
    
    byte:      0         1        2       3         42
    value: [  &2B  ][   &01   ][ &00 ][ byte ]···[ byte ]
           ( length)(DBCASTAPI)(DCRAW)(   payload data  )
    
Possible error/status values:
| Code     | Reason                                      |
|----------|---------------------------------------------|
|`CMDOK`   | Packet is added to the buffer successfully. |
|`CMDBUSY` | Data broadcast buffer is currently full.    |
|`CMDERR`  | No or invalid data channel selected.        |

#### DCFORMATA - Push a format A data broadcast packet to buffer - version 1.0.0 up:
This command encodes and inserts a format A data broadcast packet for transmission. It takes addressing and flag bytes, a variable length data payload, and calculates the relevant padding bytes and checksum to generate a correctly formatted packet.
The fourth byte holds the *IAL* in `bits 4-7` and *RI*, *CI*, and *DL* flags in `bits 1-3`. `Bit 0` is reserved.
The next three bytes hold a 24 bit *Service Packet Address*. Least significant byte first (little endian).
The next two bytes hold the *repeat* and *continuity indicator* bytes respectively.
The remaining bytes are the data broadcast *User Data Group* to be transmitted. This is a variable length, the maximum size of which depends on the *IAL* and flags selected, and whether any dummy bytes are required.
    
    byte:      0        1          2           3          4        5        6       7     8      9         9+n
    value: [  9+n ][   &01   ][   &01   ][ IAL+Flags ][ b0-7 ][ b8-15 ][ b16-23 ][ RI ][ CI ][ byte ]···[ byte ]
           (length)(DBCASTAPI)(DCFORMATA)             ( Service Packet Address  )            (   payload data  )
            
The command returns a status/error code followed by one byte containing the number of payload bytes encoded.
Possible error/status values:
| Code     | Reason                                                                  |
|----------|-------------------------------------------------------------------------|
|`CMDOK`   |Packet is added to the buffer successfully.                              |
|`CMDTRUNC`|Packet was added to data broadcast buffer but payload data was truncated.|
|`CMDBUSY` |Data broadcast buffer is currently full.                                 |
|`CMDERR`  |No or invalid data channel selected.                                     |

#### DCFORMATB - Push a format B data broadcast payload to buffer - version 1.1.0 up:
This command encodes and inserts a format B data broadcast payload for transmission. It takes addressing and flag bits, and one half of a 490 byte application data bundle payload. Two DCFORMATB invocations are therefore required to inject each application data bundle.
The fourth byte holds the *AN* in `bits 0-1`, *AI* in `bits 2-5`, and a *payload half flag* in `bit 7`. `bit 6` is reserved.
The remaining bytes contain one half of the application data bundle. The first 245 bytes are loaded when the payload half flag is clear, and the remaining 245 bytes when set. The halves must be loaded in order, and the addressing bits of the two invocations must match. If an invalid second payload half is submitted, an error will be returned and the buffer is cleared.
    
    byte:      0        1          2            3          4         248
    value: [  &F9 ][   &01   ][   &02   ][ AN+AI+Flags ][ byte ]···[ byte ]
           (length)(DBCASTAPI)(DCFORMATB)               ( 1/2 data bundle )
    
Only one application data bundle may be buffered at a time, and subsequent invocations will return a busy state until all packets have been moved to the data broadcast transmission buffer.

The command returns a status/error code.
Possible error/status values:
| Code     | Reason                                                                  |
|----------|-------------------------------------------------------------------------|
|`CMDOK`   |Payload half loaded successfully.                                        |
|`CMDBUSY` |IDL B payload buffer is currently full.                                  |
|`CMDERR`  |No or invalid data channel selected, or invalid arguments.               |

### CONFIGAPI - VBIT2 configuration API command - version 1.0.0 up:
The third byte selects a sub-command. The following sub-command bytes are defined:

|Byte | Mnemonic   | Description                             |
|-----|------------|-----------------------------------------|
|`&00`|`CONFRAFLAG`| Get/Set row adaptive transmission flag. |
|`&01`|`CONFRBYTES`| Get/Set BSDP reserved bytes.            |
|`&02`|`CONFSTATUS`| Get/Set BSDP status message.            |
|`&03`|`CONFHEADER`| Get/Set page header template.           |
|`&04`|`CONFENHANC`| Get/Set/Delete magazine enhancements.   |

Undefined sub-commands return `CMDERR`.
`CONFIGAPI` commands are only valid for channel 0.

#### CONFRAFLAG - Get/Set row adaptive transmission flag - version 1.0.0 up:
This command allows row adaptive transmission to be enabled/disabled.
The state of bit 0 of the fourth byte sets or clears the flag.

    byte:      0        1           2          3
    value: [  &04 ][   &02   ][    &00   ][ &00/&01 ]
           (length)(CONFIGAPI)(CONFRAFLAG)(   flag  )

The current state can be returned without changing the flag by sending the command and sub-command numbers only.

The command returns a status/error code followed the state of the row adaptive transmission flag.
Possible error/status values:
| Code   | Reason                 |
|--------|------------------------|
|`CMDOK` |Command successful.     |
|`CMDERR`|Invalid command length. |

#### CONFRBYTES - Get/Set BSDP reserved bytes - version 1.0.0 up:
This command allows the four reserved bytes of the packet 8/30 Format 1 BSDP to be modified.
The command takes exactly four bytes to be inserted into the BSDP in transmission order.

    byte:      0        1           2         3       4       5       6
    value: [  &07 ][   &02   ][    &01   ][ byte ][ byte ][ byte ][ byte ]
           (length)(CONFIGAPI)(CONFRBYTES)(     BSDP reserved bytes      )

The current value can be returned without modification by sending the command and sub-command numbers only.

The command returns a status/error code followed the value of the four reserved bytes.
Possible error/status values:
| Code   | Reason                 |
|--------|------------------------|
|`CMDOK` |Command successful.     |
|`CMDERR`|Invalid command length. |

#### CONFSTATUS - Get/Set BSDP status message - version 1.0.0 up:
This command allows the Broadcast Service Data Packet status message text to be modified.
The command requires exactly 20 characters to be inserted into the BSDP status display string.

    byte:      0        1           2         3         22
    value: [  &17 ][   &02   ][    &02   ][ char ]···[ char ]
           (length)(CONFIGAPI)(CONFSTATUS)(  status string  )

The current string can be returned without modification by sending the command and sub-command numbers only.

The command returns a status/error code followed the value of the BSDP status string.
Possible error/status values:
| Code   | Reason                 |
|--------|------------------------|
|`CMDOK` |Command successful.     |
|`CMDERR`|Invalid command length. |

#### CONFHEADER - Get/Set page header template - version 1.0.0 up:
This command allows the page header template to be modified.
The template string must be exactly 32 characters containing a VBIT2 page header template.

Set service-wide page header template:

    byte:      0        1           2         3         34
    value: [  &23 ][   &02   ][    &03   ][ char ]···[ char ]
           (length)(CONFIGAPI)(CONFHEADER)( template string )

The current template string can be returned by sending the command and sub-command numbers only.

Possible error/status values:
| Code   | Reason                 |
|--------|------------------------|
|`CMDOK` |Command successful.     |
|`CMDERR`|Invalid command length. |

A custom header template override can also be set for individual magazines:

    byte:      0        1           2          3         4         35
    value: [  &23 ][   &02   ][    &03   ][ &00-&07 ][ char ]···[ char ]
           (length)(CONFIGAPI)(CONFHEADER)(mag+flags)( template string )

Byte 3 contains the magazine number in bits 0-2, and flags in bits 5-7. The delete flag is bit 7, other bits are reserved and should be cleared.

The current custom header template can be returned by sending the command and sub-command numbers followed by the magazine number. A custom header can be removed from a magazine in the same way with the delete flag set.

Possible error/status values:
| Code     | Reason                        |
|----------|-------------------------------|
|`CMDOK`   | Command successful.           |
|`CMDNOENT`| No custom header in magazine. |
|`CMDERR`  | Invalid command length.       |

#### CONFENHANC - Get/Set/Delete magazine enhancements - version 1.0.0 up:
This command allows modification of magazine enhancement packets.
The command can read, add or overwrite, or delete a magazine enhancement packet.

Byte 3 contains the magazine number in bits 0-2, and flags in bits 3-7. The delete flag is bit 7, other bits are reserved and should be cleared.

Read enhancement packet data:

    byte:      0        1           2          3           4
    value: [  &05 ][   &03   ][    &04   ][ &00-&07 ][  &00-&0F  ]
           (length)(CONFIGAPI)(CONFENHANC)(mag+flags)(designation)
                                                         code

Write enhancement packet data:

    byte:      0        1           2          3         4         43
    value: [  &2C ][   &03   ][    &04   ][ &00-&07 ][ byte ]···[ byte ]
           (length)(CONFIGAPI)(CONFENHANC)(mag+flags)(   packet data   )

Delete enhancement packet:

    byte:      0         1           2          3           4*
    value: [&04/05*][   &03   ][    &04   ][ &80-&87 ][  &00-&0F  ]
           ( length)(CONFIGAPI)(CONFENHANC)(mag+flags)(designation)
                                                          code
    * Adding designation code byte will delete only the specified packet.
      Otherwise all enhancement packets are removed for the specified magazine.

Packet data is read/written in internal vbit2 line encoding format.
Bits 0-3 of the first byte hold the designation code, and the remaining 39 bytes hold six bits of triplet data each in bits 0-5.

Possible error/status values:
| Code     | Reason                        |
|----------|-------------------------------|
|`CMDOK`   | Command successful.           |
|`CMDNOENT`| Enhancement packet not found. |
|`CMDERR`  | Invalid command length.       |

### PAGESAPI - Page data API command - version 1.0.0 up:
The third byte selects a sub-command. The following sub-command bytes are defined:
|Byte | Mnemonic   | Description                             |
|-----|------------|-----------------------------------------|
|`&00`|`PAGEDELETE`| Remove a page from the service.         |
|`&01`|`PAGEOPEN`  | Open a page for updating.               |
|`&02`|`PAGESETSUB`| Select sub-page.                        |
|`&03`|`PAGEDELSUB`| Delete sub-page.                        |
|`&04`|`PAGECLOSE` | Close updated page.                     |
|`&05`|`PAGEFANDC` | Get/Set page function and coding.       |
|`&06`|`PAGEOPTNS` | Get/Set sub-page options.               |
|`&07`|`PAGEROW`   | Read/Write/Delete a page row.           |
|`&08`|`PAGELINKS` | Get/Set Fastext link data               |

Undefined sub-commands return `CMDERR`.
`PAGESAPI` commands are only valid for channel 0.

#### PAGEDELETE - Remove a page from the service - version 1.0.0 up:
This command marks a page for deletion from the teletext service by page number.
The command takes two bytes containing the page number to be removed.

    byte:      0        1           2         3         4
    value: [  &05 ][   &03  ][    &00   ][   0-7  ][ &00-&FF ]
           (length)(PAGESAPI)(PAGEDELETE)(magazine)(   page  )

If the client already has a page open, it is closed implicitly by this command regardless of success/failure.

Possible error/status values:
| Code     | Reason                    |
|----------|---------------------------|
|`CMDOK`   | Page marked for deletion. |
|`CMDNOENT`| Page not found.           |
|`CMDERR`  | Invalid command length.   |

#### PAGEOPEN - Open a page for updating - version 1.0.0 up:
This command opens a page for modification by page number. If the page does not exist it will be created.
The command takes two bytes containing the page number to be opened, followed by an optional flag to designate the page as a *'one shot'* transmission.
While a page is open for updating, it is locked and held back from the page transmission cycle. Transmission resumes only once the page has been closed. The page should therefore be updated and closed as quickly as possible.
Pages with the *OneShot* flag set will be transmitted exactly once and then held until the flag is cleared. Only being transmitted whenever subsequent modifications are made. If a *OneShot* page has multiple sub-pages, the most recently modified sub-page is queued for transmission.
Once a *OneShot* page has been queued for transmission, attempts to re-open it will return `CMDBUSY` until transmission has occurred.

    byte:      0        1         2         3         4          5
    value: [  &06 ][   &03  ][   &01  ][   0-7  ][ &00-&FF ][ &00/&01 ]
           (length)(PAGESAPI)(PAGEOPEN)(magazine)(   page  )( OneShot )

If the client already has a page open, it is closed implicitly by this command regardless of success/failure.

Possible error/status values:
| Code    | Reason                                         |
|---------|------------------------------------------------|
|`CMDOK`  | Page successfully opened.                      |
|`CMDBUSY`| Page is currently locked against modification. |
|`CMDERR` | Invalid page number or command length.         |

#### PAGESETSUB - Select sub-page - version 1.0.0 up:
This command sets the active sub-page within the currently open page by its sub-code. If the sub-page does not exist it will be created.
The command takes two bytes containing the sub-code of the sub-page to be set. The sub-code is passed with the most significant byte first (big endian).

    byte:      0        1          2         3       4
    value: [  &05 ][   &03  ][    &02   ][ b8-15 ][ b0-7 ]
           (length)(PAGESAPI)(PAGESETSUB)( page sub-code )

If the current page has the *OneShot* flag set, this sub-page will be queued for transmission.

If a page is currently open, the error/status code is followed by two bytes containing the number of sub-pages in the current page, most significant byte first (big endian).
Possible error/status values:
| Code   | Reason                                                  |
|--------|---------------------------------------------------------|
|`CMDOK` | Sub-page successfully set.                              |
|`CMDERR`| Invalid sub-code or command length, or no page is open. |

#### PAGEDELSUB - Delete sub-page - version 1.0.0 up:
This command deletes a sub-page within the currently open page by sub-code. It also clears the active subpage.
The command takes two bytes containing the sub-code of the page to be removed. The sub-code is passed with the most significant byte first (big endian).

    byte:      0        1          2         3       4
    value: [  &05 ][   &03  ][    &03   ][ b8-15 ][ b0-7 ]
           (length)(PAGESAPI)(PAGEDELSUB)( page sub-code )

If a page is currently open, the error/status code is followed by two bytes containing the number of sup-pages in the current page, most significant byte first (big endian).
Possible error/status values:
| Code     | Reason                                                  |
|----------|---------------------------------------------------------|
|`CMDOK`   | Sub-page successfully deleted.                          |
|`CMDNOENT`| Sub-page not found.                                     |
|`CMDERR`  | Invalid sub-code or command length, or no page is open. |

#### PAGECLOSE - Close updated page - version 1.0.0 up:
This command closes the current page and returns it to the page transmission cycle.
The command takes no arguments.

    byte:      0        1         2
    value: [  &05 ][   &03  ][   &04   ]
           (length)(PAGESAPI)(PAGECLOSE)

Possible error/status values:
| Code     | Reason                                          |
|----------|-------------------------------------------------|
|`CMDOK`   | Page closed and returned to transmission cycle. |
|`CMDNOENT`| No currently open page.                         |
|`CMDERR`  | Invalid command length.                         |

#### PAGEFANDC - Get/Set page function and coding - version 1.0.0 up:
This command allows modification of the page function and coding values of the current page.
The command takes two bytes, containing a page function and page coding number respectively.

    byte:      0        1         2          3          4
    value: [  &05 ][   &03  ][   &05   ][ &00-&0B ][ &00-&05 ]
           (length)(PAGESAPI)(PAGEFANDC)( function)(  coding )

The current values can be returned without modification by sending the command and sub-command numbers only.

The command returns a status/error code followed by the page function and coding values.
Possible error/status values:
| Code   | Reason                                      |
|--------|---------------------------------------------|
|`CMDOK` | Command successful.                         |
|`CMDERR`| Invalid command length, or no page is open. |

#### PAGEOPTNS - Get/Set sub-page options - version 1.0.0 up:
This command allows modification of the page state, region, cycle time, and cycle mode.
The command takes two bytes containing a VBIT2 page status value followed by three bytes containing a page region, cycle time value, and cycle mode flag respectively. The page status is passed with the most significant byte first (big endian).
Cycle time represents seconds when cycle mode flag is set, or number of magazine cycles when clear.

    byte:      0        1         2         3       4         5          6          7
    value: [  &05 ][   &03  ][   &06   ][ b8-15 ][ b0-7 ][ &00-&0A ][ &00-&FF ][ &00/&01 ]
           (length)(PAGESAPI)(PAGEOPTNS)(  page status  )(  region )(   time  )(   mode  )

The current values can be returned without modification by sending the command and sub-command numbers only.

If a sub-page is currently selected, the command returns a status/error code followed by the page status, region, cycle time, and cycle mode values.
Possible error/status values:
| Code   | Reason                                             |
|--------|----------------------------------------------------|
|`CMDOK` | Command successful.                                |
|`CMDERR`| Invalid command length, or no sub-page is selected.|

#### PAGEROW - Read/Write/Delete a page row - version 1.0.0 up:
This command allows modification of a page row within the active sub-page.
The command can read, add or overwrite, or delete a row. If a row number greater than 25 is given, the row is selected by designation code. Valid row numbers are 1-28.

Byte 3 contains the row number in bits 0-4, and flags in bits 5-7. The delete flag is bit 7, other bits are reserved and should be cleared.

Read row:

    byte:      0         1        2         3           4*
    value: [&04/05*][   &03  ][  &07  ][ &01-&1C ][  &00-&0F  ]
           ( length)(PAGESAPI)(PAGEROW)(row+flags)(designation)
                                                         code
    * Designation code must be added for row numbers greater than 25

Write row:

    byte:      0        1        2         3         4         43
    value: [  &2C ][   &03  ][  &07  ][ &01-&1C ][ byte ]···[ byte ]
           (length)(PAGESAPI)(PAGEROW)(row+flags)(     row data    )

Delete row:

    byte:      0         1        2         3           4*
    value: [&04/05*][   &03  ][  &07  ][ &81-&9C ][  &00-&0F  ]
           ( length)(PAGESAPI)(PAGEROW)(row+flags)(designation)
                                                       code
    * Adding designation code for row numbers greater than 25 will delete only the specified designation
      code. Otherwise all rows with the specified row number are removed.

Row data is written/read in vbit2 line encoding format appropriate to row number, type, and page coding in use.
For rows 1-25 of normal Level One Pages (page coding 0) this is plan 7-bit character data.

If a sub-page is currently selected, the command returns a status/error code followed by 40 bytes of row data for a successful read operation.
Possible error/status values:
| Code     | Reason                                              |
|----------|-----------------------------------------------------|
|`CMDOK`   | Command successful.                                 |
|`CMDNOENT`| Row does not exist.                                 |
|`CMDERR`  | Invalid command length, or no sub-page is selected. |

#### PAGELINKS - Get/Set Fastext link data - version 1.0.0 up:
This command provides a convenient way to read and modify the Fastext linking data of the active subpage.

Set link page numbers:

    byte:      0        1         2          3          4             13        14
    value: [  &0F ][   &03  ][   &08   ][ &00-&07 ][ &00-&FF ]...[ &00-&07 ][ &00-&FF ]
           (length)(PAGESAPI)(PAGELINKS)( link 0 page number )   ( link 5 page number )

The command can be extended to set a specific sub-code for each link:

    byte:      0        1         2        3 - 14       15      16          25      26
    value: [  &1B ][   &03  ][   &08   ]( as above )[ b8-15 ][ b0-7 ]...[ b8-15 ][ b0-7 ]
           (length)(PAGESAPI)(PAGELINKS)            (link 0 sub-code)   (link 5 sub-code)

The current page and sub-code link values can be returned without modification by sending the command and sub-command numbers only.
Possible error/status values:
| Code     | Reason                                              |
|----------|-----------------------------------------------------|
|`CMDOK`   | Command successful.                                 |
|`CMDNOENT`| Fastext link data row does not exist.               |
|`CMDERR`  | Invalid command length, or no sub-page is selected. |

### GETAPIVER - Get API version the server implements - version 1.0.0 up:
This command requests the API version number on the server.

    byte:      0        1
    value: [  &03 ][   &04   ]
           (length)(GETAPIVER)

The command returns three 8-bit values containing major version, minor version, and patch level respectively.
Possible error/status values:
| Code   | Reason                  |
|--------|-------------------------|
|`CMDOK` | Command successful.     |
|`CMDERR`| Invalid command length. |

