-- Student number: 400431308, MacID: khanr144;
-- Query 1
WITH saleOffer AS (
	SELECT sid, customer_id, offer_id, seller_id, product, price
	FROM SALE,OFFER
	WHERE OID = offer_id)
,
productOverviews AS (
	SELECT s.product,AVG(price) AS avgprice,AVG(p.score) AS avgscore,COUNT(DISTINCT sid) AS numsales,COUNT(DISTINCT seller_id) AS numsellers
	FROM saleOffer s, preview p
	WHERE s.product=p.product
	GROUP BY s.product)	
SELECT * FROM productOverviews

-- Query 2
WITH samesellers AS (SELECT a.seller_id AS seller1,b.seller_id AS seller2,a.product AS product1,b.product AS product2,a.price AS price1,b.price AS price2
FROM offer a INNER JOIN offer b
ON a.price = b.price AND a.product = b.product AND a.seller_id<>b.seller_id),
sellertuples AS (SELECT a.seller_id AS seller1,b.seller_id AS seller2,a.product AS product1,b.product AS product2,a.price AS price1,b.price AS price2
FROM offer a INNER JOIN offer b
ON a.seller_id<>b.seller_id),
samesellersp AS (SELECT DISTINCT a.seller_id AS seller1,b.seller_id AS seller2,a.product AS product1,b.product
FROM offer a, offer b
WHERE a.product = b.product AND a.seller_id<>b.seller_id)
SELECT DISTINCT seller1 AS aid,seller2 AS bid FROM samesellers

-- Query 3
WITH salesreviews AS (SELECT * FROM sale,sreview
WHERE sid = sale_id),
allreviewids AS (SELECT customer_id AS id FROM salesreviews
UNION ALL 
SELECT user_id AS id FROM preview),
idcount AS (SELECT id, COUNT(*) AS id_count
FROM allreviewids
GROUP BY id),
atleast10 AS (SELECT * FROM idcount
WHERE id_count > 9),
allinfo AS (SELECT * FROM sale,OFFER
WHERE offer_id = oid),
atleast10info AS (SELECT * FROM allinfo
WHERE customer_id IN (
SELECT id FROM atleast10)),
distinctproduct AS (SELECT customer_id,COUNT(DISTINCT product) AS num_products
FROM atleast10info
GROUP BY customer_id),
distinctseller AS (SELECT customer_id,COUNT(DISTINCT seller_id) AS num_sellers
FROM atleast10info
GROUP BY customer_id),
distinctpreview AS (SELECT user_id,COUNT(DISTINCT product) AS num_preview FROM PREVIEW
GROUP BY user_id),
distinctsreview AS (SELECT customer_id,COUNT(DISTINCT sale_id) AS num_sreview FROM salesreviews
GROUP BY customer_id),
variables AS (SELECT r.user_id, num_products AS NP, num_sellers AS NS, num_preview AS RP, num_sreview AS RS FROM distinctproduct p,distinctseller s,distinctpreview r,distinctsreview v
WHERE p.customer_id = s.customer_id AND s.customer_id = r.user_id AND r.user_id = v.customer_id),
variables2 AS (SELECT *, CAST(RP AS DECIMAL)/CAST(NP AS DECIMAL) AS a, CAST(RS AS DECIMAL)/CAST(NS AS DECIMAL) AS b FROM variables),
variables3 AS (SELECT USER_ID AS uid, 10*(a+b) AS css FROM variables2)
SELECT * FROM variables3

-Query 4
WITH salesreview AS (SELECT w.sale_id,w.score,s.offer_id,s.time,o.seller_id
FROM sale s,sreview w, offer o
WHERE w.sale_id = s.sid AND s.offer_id=o.oid
ORDER BY o.seller_id ASC, s.time DESC),
rowNo AS (SELECT *,
  ( select count (*)
     from salesreview u
     where
       t.SELLER_ID = u.SELLER_ID AND t.time >= u.time
  ) as ROW_NUMBER
from salesreview t),
mult AS (SELECT *,score*ROW_NUMBER AS SxO FROM rowNo),
upperfraction AS (SELECT SELLER_ID,SUM (SxO) AS total FROM mult
GROUP BY seller_id),
lowerfraction AS (SELECT seller_id, count(*) AS n FROM salesreview
GROUP BY seller_id),
lowerfraction2 AS (SELECT *, n + 1 AS n2 FROM lowerfraction),
upperfraction2 AS (SELECT *,2*total AS upper FROM upperfraction),
lowerfraction3 AS (SELECT *,n*n2 AS lower FROM lowerfraction2),
fraction AS (SELECT u.seller_id AS uid,CAST(upper AS DOUBLE)/CAST(lower AS DOUBLE) AS tsr FROM  upperfraction2 u,lowerfraction3 l
WHERE u.seller_id=l.seller_id)
SELECT * FROM fraction
