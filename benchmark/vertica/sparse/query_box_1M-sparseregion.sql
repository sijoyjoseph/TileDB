-- 17536 rows
drop table t1;
select clear_caches();
select clear_caches();
select clear_caches();
\timing
select X,Y INTO TEMP TABLE t1 ON COMMIT PRESERVE ROWS from vessel_traffic where x<92320400 and y>=130000000 and y<136000000;
--\timing
select count(*) from t1;
