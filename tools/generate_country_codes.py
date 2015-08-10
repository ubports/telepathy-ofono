# originally available at https://github.com/musalbas/mcc-mnc-table/blob/master/get-mcc-mnc-table-csv.py
import re
import urllib2

html = urllib2.urlopen('http://mcc-mnc.com/').read()
td_re = re.compile('<td>([^<]*)</td>'*6)
codes = {}
tbody_start = False
for line in html.split('\n'):
    if '<tbody>' in line:
        tbody_start = True
    elif '</tbody>' in line:
        break
    elif tbody_start:
        td_search = td_re.search(line)
        mcc = int(td_search.group(1))
        isoCountryCode = td_search.group(3).upper()
        if (len(isoCountryCode) != 2):
            continue
        codes[mcc] = isoCountryCode

for mcc in codes:
    print str(mcc) + ":" + codes[mcc]
