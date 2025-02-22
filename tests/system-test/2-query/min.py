from wsgiref.headers import tspecials
from util.log import *
from util.cases import *
from util.sql import *
import numpy as np


class TDTestCase:
    def init(self, conn, logSql):
        tdLog.debug("start to execute %s" % __file__)
        tdSql.init(conn.cursor())

        self.rowNum = 10
        self.ts = 1537146000000

    def run(self):
        tdSql.prepare()

        intData = []
        floatData = []

        tdSql.execute('''create table stb(ts timestamp, col1 tinyint, col2 smallint, col3 int, col4 bigint, col5 float, col6 double,
                    col7 bool, col8 binary(20), col9 nchar(20), col11 tinyint unsigned, col12 smallint unsigned, col13 int unsigned, col14 bigint unsigned) tags(loc nchar(20))''')
        tdSql.execute("create table stb_1 using stb tags('beijing')")
        tdSql.execute('''create table ntb(ts timestamp, col1 tinyint, col2 smallint, col3 int, col4 bigint, col5 float, col6 double,
                    col7 bool, col8 binary(20), col9 nchar(20), col11 tinyint unsigned, col12 smallint unsigned, col13 int unsigned, col14 bigint unsigned)''')
        for i in range(self.rowNum):
            tdSql.execute("insert into ntb values(%d, %d, %d, %d, %d, %f, %f, %d, 'taosdata%d', '涛思数据%d', %d, %d, %d, %d)"
                        % (self.ts + i, i + 1, i + 1, i + 1, i + 1, i + 0.1, i + 0.1, i % 2, i + 1, i + 1, i + 1, i + 1, i + 1, i + 1))
            intData.append(i + 1)
            floatData.append(i + 0.1)
        for i in range(self.rowNum):
            tdSql.execute("insert into stb_1 values(%d, %d, %d, %d, %d, %f, %f, %d, 'taosdata%d', '涛思数据%d', %d, %d, %d, %d)"
                        % (self.ts + i, i + 1, i + 1, i + 1, i + 1, i + 0.1, i + 0.1, i % 2, i + 1, i + 1, i + 1, i + 1, i + 1, i + 1))
            intData.append(i + 1)
            floatData.append(i + 0.1)

        # max verifacation
        tdSql.error("select min(ts) from stb_1")
        tdSql.error("select min(ts) from db.stb_1")
        tdSql.error("select min(col7) from stb_1")
        tdSql.error("select min(col7) from db.stb_1")
        tdSql.error("select min(col8) from stb_1")
        tdSql.error("select min(col8) from db.stb_1")
        tdSql.error("select min(col9) from stb_1")
        tdSql.error("select min(col9) from db.stb_1")
        # tdSql.error("select min(a) from stb_1")
        # tdSql.error("select min(1) from stb_1")
        tdSql.error("select min(now()) from stb_1")
        tdSql.error("select min(count(c1),count(c2)) from stb_1")

        tdSql.query("select min(col1) from stb_1")
        tdSql.checkData(0, 0, np.min(intData))
        tdSql.query("select min(col1) from db.stb_1")
        tdSql.checkData(0, 0, np.min(intData))
        tdSql.query("select min(col2) from stb_1")
        tdSql.checkData(0, 0, np.min(intData))
        tdSql.query("select min(col2) from db.stb_1")
        tdSql.checkData(0, 0, np.min(intData))
        tdSql.query("select min(col3) from stb_1")
        tdSql.checkData(0, 0, np.min(intData))
        tdSql.query("select min(col3) from db.stb_1")
        tdSql.checkData(0, 0, np.min(intData))
        tdSql.query("select min(col4) from stb_1")
        tdSql.checkData(0, 0, np.min(intData))
        tdSql.query("select min(col4) from db.stb_1")
        tdSql.checkData(0, 0, np.min(intData))
        tdSql.query("select min(col11) from stb_1")
        tdSql.checkData(0, 0, np.min(intData))
        tdSql.query("select min(col11) from db.stb_1")
        tdSql.checkData(0, 0, np.min(intData))
        tdSql.query("select min(col12) from stb_1")
        tdSql.checkData(0, 0, np.min(intData))
        tdSql.query("select min(col12) from db.stb_1")
        tdSql.checkData(0, 0, np.min(intData))
        tdSql.query("select min(col13) from stb_1")
        tdSql.checkData(0, 0, np.min(intData))
        tdSql.query("select min(col13) from db.stb_1")
        tdSql.checkData(0, 0, np.min(intData))
        tdSql.query("select min(col14) from stb_1")
        tdSql.checkData(0, 0, np.min(intData))
        tdSql.query("select min(col14) from db.stb_1")
        tdSql.checkData(0, 0, np.min(intData))
        tdSql.query("select min(col5) from stb_1")
        tdSql.checkData(0, 0, np.min(floatData))
        tdSql.query("select min(col5) from db.stb_1")
        tdSql.checkData(0, 0, np.min(floatData))
        tdSql.query("select min(col6) from stb_1")
        tdSql.checkData(0, 0, np.min(floatData))
        tdSql.query("select min(col6) from db.stb_1")
        tdSql.checkData(0, 0, np.min(floatData))
        tdSql.query("select min(col1) from stb_1 where col2>=5")
        tdSql.checkData(0,0,5)


        tdSql.error("select min(ts) from stb_1")
        tdSql.error("select min(ts) from db.stb_1")
        tdSql.error("select min(col7) from stb_1")
        tdSql.error("select min(col7) from db.stb_1")
        tdSql.error("select min(col8) from stb_1")
        tdSql.error("select min(col8) from db.stb_1")
        tdSql.error("select min(col9) from stb_1")
        tdSql.error("select min(col9) from db.stb_1")
        # tdSql.error("select min(a) from stb_1")
        # tdSql.error("select min(1) from stb_1")
        tdSql.error("select min(now()) from stb_1")
        tdSql.error("select min(count(c1),count(c2)) from stb_1")

        tdSql.query("select min(col1) from stb")
        tdSql.checkData(0, 0, np.min(intData))
        tdSql.query("select min(col1) from db.stb")
        tdSql.checkData(0, 0, np.min(intData))
        tdSql.query("select min(col2) from stb")
        tdSql.checkData(0, 0, np.min(intData))
        tdSql.query("select min(col2) from db.stb")
        tdSql.checkData(0, 0, np.min(intData))
        tdSql.query("select min(col3) from stb")
        tdSql.checkData(0, 0, np.min(intData))
        tdSql.query("select min(col3) from db.stb")
        tdSql.checkData(0, 0, np.min(intData))
        tdSql.query("select min(col4) from stb")
        tdSql.checkData(0, 0, np.min(intData))
        tdSql.query("select min(col4) from db.stb")
        tdSql.checkData(0, 0, np.min(intData))
        tdSql.query("select min(col11) from stb")
        tdSql.checkData(0, 0, np.min(intData))
        tdSql.query("select min(col11) from db.stb")
        tdSql.checkData(0, 0, np.min(intData))
        tdSql.query("select min(col12) from stb")
        tdSql.checkData(0, 0, np.min(intData))
        tdSql.query("select min(col12) from db.stb")
        tdSql.checkData(0, 0, np.min(intData))
        tdSql.query("select min(col13) from stb")
        tdSql.checkData(0, 0, np.min(intData))
        tdSql.query("select min(col13) from db.stb")
        tdSql.checkData(0, 0, np.min(intData))
        tdSql.query("select min(col14) from stb")
        tdSql.checkData(0, 0, np.min(intData))
        tdSql.query("select min(col14) from db.stb")
        tdSql.checkData(0, 0, np.min(intData))
        tdSql.query("select min(col5) from stb")
        tdSql.checkData(0, 0, np.min(floatData))
        tdSql.query("select min(col5) from db.stb")
        tdSql.checkData(0, 0, np.min(floatData))
        tdSql.query("select min(col6) from stb")
        tdSql.checkData(0, 0, np.min(floatData))
        tdSql.query("select min(col6) from db.stb")
        tdSql.checkData(0, 0, np.min(floatData))
        tdSql.query("select min(col1) from stb where col2>=5")
        tdSql.checkData(0,0,5)


        tdSql.error("select min(ts) from ntb")
        tdSql.error("select min(ts) from db.ntb")
        tdSql.error("select min(col7) from ntb")
        tdSql.error("select min(col7) from db.ntb")
        tdSql.error("select min(col8) from ntb")
        tdSql.error("select min(col8) from db.ntb")
        tdSql.error("select min(col9) from ntb")
        tdSql.error("select min(col9) from db.ntb")
        # tdSql.error("select min(a) from stb_1")
        # tdSql.error("select min(1) from stb_1")
        tdSql.error("select min(now()) from ntb")
        tdSql.error("select min(count(c1),count(c2)) from ntb")

        tdSql.query("select min(col1) from ntb")
        tdSql.checkData(0, 0, np.min(intData))
        tdSql.query("select min(col1) from db.ntb")
        tdSql.checkData(0, 0, np.min(intData))
        tdSql.query("select min(col2) from ntb")
        tdSql.checkData(0, 0, np.min(intData))
        tdSql.query("select min(col2) from db.ntb")
        tdSql.checkData(0, 0, np.min(intData))
        tdSql.query("select min(col3) from ntb")
        tdSql.checkData(0, 0, np.min(intData))
        tdSql.query("select min(col3) from db.ntb")
        tdSql.checkData(0, 0, np.min(intData))
        tdSql.query("select min(col4) from ntb")
        tdSql.checkData(0, 0, np.min(intData))
        tdSql.query("select min(col4) from db.ntb")
        tdSql.checkData(0, 0, np.min(intData))
        tdSql.query("select min(col11) from ntb")
        tdSql.checkData(0, 0, np.min(intData))
        tdSql.query("select min(col11) from db.ntb")
        tdSql.checkData(0, 0, np.min(intData))
        tdSql.query("select min(col12) from ntb")
        tdSql.checkData(0, 0, np.min(intData))
        tdSql.query("select min(col12) from db.ntb")
        tdSql.checkData(0, 0, np.min(intData))
        tdSql.query("select min(col13) from ntb")
        tdSql.checkData(0, 0, np.min(intData))
        tdSql.query("select min(col13) from db.ntb")
        tdSql.checkData(0, 0, np.min(intData))
        tdSql.query("select min(col14) from ntb")
        tdSql.checkData(0, 0, np.min(intData))
        tdSql.query("select min(col14) from db.ntb")
        tdSql.checkData(0, 0, np.min(intData))
        tdSql.query("select min(col5) from ntb")
        tdSql.checkData(0, 0, np.min(floatData))
        tdSql.query("select min(col5) from db.ntb")
        tdSql.checkData(0, 0, np.min(floatData))
        tdSql.query("select min(col6) from ntb")
        tdSql.checkData(0, 0, np.min(floatData))
        tdSql.query("select min(col6) from db.ntb")
        tdSql.checkData(0, 0, np.min(floatData))
        tdSql.query("select min(col1) from ntb where col2>=5")
        tdSql.checkData(0,0,5)


    def stop(self):
        tdSql.close()
        tdLog.success("%s successfully executed" % __file__)

tdCases.addWindows(__file__, TDTestCase())
tdCases.addLinux(__file__, TDTestCase())
