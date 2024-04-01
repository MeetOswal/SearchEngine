from bs4 import BeautifulSoup
import requests
import re
import time
import urllib.robotparser
import random
from langdetect import detect
import csv
from collections import Counter, defaultdict
import datetime

#google package to for searching the seed pages
try:
    from googlesearch import search
except ImportError:
    print("No module named 'google' found")
    
visited = [] #list of urls which are crawled
query = "Machine Learning" #query to google
queue = [] #list of urls got from pages crawled
language = [] #language detected in sampled pages
sample_probability = 0.15 # probability that a page will be sampled
shuffle_probability = 0.2 # probabilty of queue getting shuffled to jump location from 1 url to another
totalSize = 0 # total size of data crawled in kb
csv_file_name = 'logs2.csv' # log of web crawler
sampleSize = 797 # number of samples genrated

start_time = time.time() #start of code

#getting data from google search
for j in search(query, tld="com", num=15, stop=10, pause=2):
    queue.append(j)

url = queue[0]
#setting for robots.txt pareser
rp = urllib.robotparser.RobotFileParser()

#function that checks in robots.txt if the page is crawleable
def permission(url):
    root_url = url.split('/')
    rp.set_url(f'{root_url[0]}//{root_url[2]}/robots.txt')
    rp.read()
    permission = True
    # permission = rp.can_fetch("*", url)
    return permission

#function to fetch data from url
def getSource(url):
    print(url)
    try:
        source = requests.get(url, timeout= 5)
        print(source)
        return source
    except requests.exceptions.Timeout:
        source = None
        return source
    except requests.exceptions.RequestException as e:
        source = None
        return source
    except Exception as e:
        source = None
        return source

#function to write logs
def writeToLog(url, soup, code, content_size,sampled):

    with open(csv_file_name, mode = "a", newline="") as csv_file:
        if(sampled):
            lan = getLanguage(soup)
            language.append(lan)
        else:
            lan = " "
        row = {
            "URL": url, #url
            "Time":datetime.datetime.now(), #time of crawl
            "Status Code": code, #status sent by the server
            "Sampled": sampled, #true or false if the page is sampled
            "Language": lan, #langauge of the page
            "content size": content_size #size of the page
        }
        csv_writer = csv.DictWriter(csv_file, fieldnames=["URL", "Time", "Status Code", "Sampled", "Language", "content size"])

        csv_writer.writerow(row)
    return

#function to create a sample
def createSample(soup):
    f = open(f'samples_2\sample_{sampleSize}',"w")
    f.write(soup.prettify())
    f.close()
    return
#function to get all links from a crawled page
def getLinks(soup):
    link_list = []
    links = soup.find_all('a', attrs={'href': re.compile("^https://")})
    for i in links:
        if(i['href'] not in visited or i['href'] not in queue):
            link_list.append(i['href']) # saving all the links in a list
    return link_list

#this function selects 100 links from all the links we get from a page(if number of links on that page are more than 100)
#the functio uses dictionary to select pages from all different domains present
def sortList(urls):
    random.shuffle(urls)
    links_by_domain = defaultdict(list)
    for link in urls:
        domain = link.split("//")[1].split("/")[0]  # Extract the domain from the link
        links_by_domain[domain].append(link)

    selected_links = []

    # Prioritize links from different domains while selecting a total of 100 links
    remaining_links_count = 100

    for domain, links in links_by_domain.items():
        num_links_to_select = min(len(links), remaining_links_count)
        selected_links.extend(random.sample(links, num_links_to_select))
        remaining_links_count -= num_links_to_select

        if remaining_links_count <= 0:
            break
    
    return selected_links

#detect language of the crawled page
def getLanguage(soup):
    body_text = soup.body.get_text()
    return detect(body_text)

#main crawling starts
while sampleSize <= 1000: #run till number of pages does not reach 10000(there is a condtion to break the loop if number of sampled pages reach 1000)
    try:
        #get a number to check if the queue need to be shuffled or not, comapred to the probabilty defined above
        shuffle_number = random.random()

        #if we get premision to crawl a webiste it enters the if case or else the url is not crawled
        if(True):#permission(url)

            source = getSource(url)
            queue.pop(0)
            if(source == None):
                url = queue[0]
                continue
            content_size = len(source.content)
            content_size = content_size / 1024
            visited.append(url)
            # print('visited:', url )

            # print(len(visited),'/100')

            soup = BeautifulSoup(source.text, 'lxml')

            sample_number = random.random()

            #sample number is compared to probabilty to see if the page will be sampled or not
            if sample_number < sample_probability:
                sampleSize = sampleSize + 1
                createSample(soup)
                writeToLog(url, soup, source.status_code, content_size,True)
                totalSize = totalSize + content_size
            else:
                writeToLog(url, soup, source.status_code, content_size,False)
                totalSize = totalSize + content_size
            #get all links from the page
            url_list = getLinks(soup)
            #append the urls to queue
            queue = queue + sortList(url_list)
            # print(len(queue))

        #shuffle number is compared to probabilty to see if the queue will be shuffled or not
        # if shuffle_number < shuffle_probability:
        #     random.shuffle(queue)

        if(len(queue) == 0):
            break
        
        # set url for next interation
        url = queue[0]
        sleep = 1
        # if(rp.crawl_delay("*")):
        #sleep = rp.crawl_delay("*")
        # time.sleep(sleep)        
    except Exception as e:
        print(e)

end_time = time.time()
elapsed_time = end_time - start_time
#Print all data
print("Total Run time:",elapsed_time, "sec")
print("Total crawled data size:", totalSize, "kb")
print("Total pages Crawled:", len(visited))
counts = Counter(language) #get top languages 
print("Top Languages:", counts)
#write data to file
f = open(f'Final_Data',"w")
f.write(f'Total Run time: {elapsed_time} sec, \n Total crawled data size: {totalSize} kb, \nTotal pages Crawled: {len(visited)}, \nTop Languages: {counts}')
f.close()
