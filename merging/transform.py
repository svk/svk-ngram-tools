import re
from sys import stdin

tlds = [ "com", "org", "net", "museum", "fr", "de", "uk", "za", "ca", "us", "nz", "no", "se", "dk" ]
tldgroup = "(%s)" % "|".join( tlds )

digit = re.compile( "[0-9]" )
email = re.compile( "^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,4}$" )
fulluri = re.compile( "^[a-zA-Z]{3,4}://[a-zA-Z0-9.-]+\.[a-zA-Z]{2,4}" )
tluri = re.compile( "^[a-zA-Z]{3,4}://[a-zA-Z0-9.-]+\.[a-zA-Z]{2,4}/?$" )
domainname = re.compile( "^(?:[a-zA-Z0-9-]+\.)+" + tldgroup + "$" )
unusual_or_misspelt_domain_name = re.compile( "^(www|ftp)\.(?:[a-zA-Z0-9-]+\.)+[a-zA-Z]{2,4}" )
ip_address = re.compile( "^(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$")
tagged_hex_number = re.compile( "(0x[0-9a-fA-F]+|0[0-9a-fA-F]+h)" )


small_number = re.compile( "^[0-9]{1,2}$" )

# number types to be lumped together. matched after substitution of all digits with 0
number_range = re.compile( "0+-0+" )

def transformation( s ):
    if tagged_hex_number.match( s ):
        return "<HEX_NUMBER>"
    if ip_address.match( s ):
        return "<IP_ADDRESS>"
    if domainname.match( s ) or unusual_or_misspelt_domain_name.match(s):
        return "<DOMAIN_NAME>"
    if tluri.match( s ):
        return "<TOP_URI>"
    if fulluri.match( s ):
        return "<URI>"
    if email.match( s ):
        return "<EMAIL>"
    if small_number.match( s ): # positive!
        return s
    s = re.sub( digit, "0", s )
    return s

if __name__ == '__main__':
    while True:
        v = stdin.readline()
        if not v:
            break
        key, value = v.strip().split("\t")
        key = " ".join( map( transformation, key.split() ) )
        print "%s\t%s" % (key,value)
