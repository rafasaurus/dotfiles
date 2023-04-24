#!/usr/bin/python

import calplot
import os
import numpy as np; np.random.seed(sum(map(ord, 'calplot')))
import pandas as pd
import matplotlib.pyplot as plt
import pytodotxt
from datetime import datetime, date

calculateAllTasks = False

def main():
    todotxt = pytodotxt.TodoTxt(os.path.expanduser('~')+'/todo/done.txt')
    todotxt.parse()

    completed = []
    descriptions = []

    for task in todotxt.tasks:
        if task.is_completed:
            # Don't show very early tasks
            if datetime.strptime(str(task.completion_date), '%Y-%m-%d') > datetime.strptime("2023-01-01", '%Y-%m-%d'):
                # Show only tasks that has been created and done on the same day
                if calculateAllTasks == False:
                    if task.creation_date == task.completion_date:
                        completed.append(str(task.creation_date))
                        descriptions.append(task.description)
                else:
                    completed.append(str(task.completion_date))
                    descriptions.append(task.description)

    # Convert to isoformat that calplot wants using pandas
    completed = pd.DatetimeIndex(completed)

    df = pd.DataFrame(list(zip(completed, descriptions)), 
                      columns = ['completion_date', 'description'])

    # Count how many tasks have been done each day and store in pandas Series
    events = df.pivot_table(index = ['completion_date'], aggfunc ='size')

    calplot.calplot(events, yearascending=True, vmin=-1, vmax=7, colorbar=True, linewidth=1, cmap='YlGn', suptitle=date.today())

    plt.savefig('/tmp/done.png', bbox_inches='tight')

if __name__ == "__main__":
    main()
